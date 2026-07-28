/*
 * SCSI子系统学习驱动程序
 * 用于学习目的的虚拟SCSI主机适配器驱动
 *
 * 本驱动实现了一个带内存磁盘的虚拟SCSI主机适配器，
 * 从底层向上完整演示了SCSI中间层接口。
 *
 * 涵盖的关键概念：
 * 1. SCSI主机注册 (scsi_host_alloc, scsi_add_host, scsi_scan_host)
 * 2. 命令处理 (queuecommand, scatter-gather)
 * 3. SCSI命令 (INQUIRY, TEST_UNIT_READY, READ_CAPACITY, READ/WRITE)
 * 4. Sense数据生成
 * 5. 错误处理 (eh_abort_handler, eh_host_reset_handler)
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/blkdev.h>
#include <linux/string.h>
#include <linux/scatterlist.h>
#include <linux/workqueue.h>
#include <asm-generic/unaligned.h>

#include <scsi/scsi.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_dbg.h>
#include <scsi/scsi_device.h>

#define DRV_NAME "virtual_scsi_host" /* 驱动名称 */
#define DRV_VERSION "1.0"            /* 驱动版本 */

/* 内核版本兼容：命令完成函数 */
static inline void vhost_scsi_done(struct scsi_cmnd *cmd)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 136)
    scsi_done(cmd);
#else
    cmd->scsi_done(cmd);
#endif
}

/* 虚拟磁盘配置 */
#define VIRTUAL_DISK_SIZE_MB 16    /* 虚拟磁盘大小（MB） */
#define VIRTUAL_SECTOR_SIZE 512    /* 扇区大小（字节） */
#define VIRTUAL_DISK_SECTORS ((VIRTUAL_DISK_SIZE_MB * 1024 * 1024) / VIRTUAL_SECTOR_SIZE) /* 总扇区数 */

/* 虚拟主机私有数据结构体 */
struct vhost_data {
    unsigned char *disk_buffer;      /* 内存中的虚拟磁盘缓冲区 */
    spinlock_t lock;                 /* 磁盘访问自旋锁 */
    unsigned int sectors_read;       /* 统计：已读取扇区数 */
    unsigned int sectors_written;    /* 统计：已写入扇区数 */
    struct workqueue_struct *wq;     /* 异步命令处理工作队列 */
};

/* 异步命令处理工作项结构体 */
struct vhost_work {
    struct work_struct work;         /* 工作结构体 */
    struct scsi_cmnd *cmd;           /* 关联的SCSI命令 */
};

/* 从scsi_host获取vhost_data的宏 */
#define host_to_vhost(host) ((struct vhost_data *)shost_priv(host))

/* 支持的SCSI命令操作码 */
#define OPCODE_TEST_UNIT_READY   0x00 /* 设备就绪检测(6字节) */
#define OPCODE_INQUIRY           0x12 /* 查询设备信息(6字节) */
#define OPCODE_MODE_SENSE_6      0x1A /* 模式读取(6字节) */
#define OPCODE_MODE_SENSE_10     0x5A /* 模式读取(10字节) */
#define OPCODE_MODE_SELECT_6     0x15 /* 模式配置(6字节) */
#define OPCODE_MODE_SELECT_10    0x55 /* 模式配置(10字节) */
#define OPCODE_READ_CAPACITY_10  0x25 /* 容量查询(10字节) */
#define OPCODE_READ_CAPACITY_16  0x9E /* 容量查询(16字节) */
#define OPCODE_READ_6            0x08 /* 读命令(6字节) */
#define OPCODE_READ_10           0x28 /* 读命令(10字节) */
#define OPCODE_WRITE_6           0x0A /* 写命令(6字节) */
#define OPCODE_WRITE_10          0x2A /* 写命令(10字节) */
#define OPCODE_VERIFY_10         0x2F /* 数据校验(10字节) */
#define OPCODE_SYNCHRONIZE_CACHE 0x35 /* 缓存同步(10字节) */

/*
 * INQUIRY命令响应数据 (SPC-4标准)
 * 总长度36字节，其中前4字节为标准头部，后接31字节厂商/产品信息
 */
static unsigned char inquiry_data[] = {
    /* 标准INQUIRY头部 (字节0~7) */
    0x00,       /* 外设限定符(bit 7-5)=0 设备存在, 设备类型(bit 4-0)=0 直接访问块设备，即磁盘 */
    0x00,       /* RMB(bit 7)=0 介质不可移除，其余位保留 */
    0x06,       /* 版本: 0x06 表示SPC-4 (ANSI版本) */
    0x02,       /* 响应数据格式: 0x02 标准格式，附加数据从字节4开始 */
    0x1F,       /* 附加长度 = 31 (0x1F) 表示后续31字节有效，总长度=4+1+31=36 */
    0x00, 0x00, 0x00,   /* 保留字节，填0 */

    /* 厂商名："LubanCat" (8字节) */
    0x4C, 0x75, 0x62, 0x61,   /* L u b a */
    0x6E, 0x43, 0x61, 0x74,   /* n C a t */

    /* 产品ID："VIRTUAL DISK" (16字节) */
    0x56, 0x49, 0x52, 0x54,   /* V I R T */
    0x55, 0x41, 0x4C, 0x20,   /* U A L (空格) */
    0x44, 0x49, 0x53, 0x4B,   /* D I S K */
    0x20, 0x20, 0x20, 0x20,   /* 补空格至16字节 */

    /* 产品修订级别："1.0 " (4字节) */
    0x31, 0x2E, 0x30, 0x20,   /* 1 . 0 (空格) */
};

/*
 * 为错误条件生成Sense数据
 * @param cmd: SCSI命令结构体
 * @param sense_key: 感知键
 * @param asc: 附加感知码
 * @param ascq: 附加感知码限定符
 */
static void set_sense_data(struct scsi_cmnd *cmd, int sense_key, int asc, int ascq)
{
    unsigned char *sense = cmd->sense_buffer; /* 获取Sense缓冲区指针 */
    
    memset(sense, 0, SCSI_SENSE_BUFFERSIZE);  /* 清零Sense缓冲区 */
    
    /* 固定格式Sense数据 (70h或71h) */
    sense[0] = 0x70;      /* 固定格式，当前错误 */
    sense[2] = sense_key; /* 设置感知键 */
    sense[7] = 10;        /* 附加Sense长度 */
    sense[12] = asc;      /* 设置附加感知码 */
    sense[13] = ascq;     /* 设置附加感知码限定符 */ 
}

/*
 * 执行SCSI TEST UNIT READY命令
 * @param cmd: SCSI命令结构体
 * @return: 返回处理状态
 */
static int do_test_unit_ready(struct scsi_cmnd *cmd)
{
    /* 虚拟磁盘总是就绪的 */
    cmd->result = DID_OK << 16; /* 设置命令结果为成功 */
    return 0;
}

/*
 * 执行SCSI INQUIRY命令
 * @param cmd: SCSI命令结构体
 * @return: 返回实际传输的数据长度
 */
static int do_inquiry(struct scsi_cmnd *cmd)
{
    unsigned char *cdb = cmd->cmnd;                /* 获取命令描述块指针 */
    u32 alloc_len = get_unaligned_be16(cdb + 3);   /* 主机请求的分配长度 (CDB[3:4]大端序) */
    u32 data_len = sizeof(inquiry_data);           /* 本地INQUIRY数据总长度 (u32避免min类型警告) */
    u32 copy_len = min(alloc_len, data_len);       /* 实际复制长度为请求长度与本地数据的最小值 */

    /* 设置数据传输方向: 数据从设备流向主机 */
    cmd->sc_data_direction = DMA_FROM_DEVICE;

    /* 将INQUIRY数据复制到sg列表，只复制允许的字节数 */
    scsi_sg_copy_from_buffer(cmd, inquiry_data, copy_len);

    /* 设置剩余未传输字节数：请求长度 - 实际传输长度 */
    scsi_set_resid(cmd, alloc_len - copy_len);

    cmd->result = DID_OK << 16;     /* 命令成功完成 */
    return copy_len;                /* 返回实际传输字节数 */
}

/*
 * 执行SCSI READ CAPACITY(10)命令
 * @param cmd: SCSI命令结构体
 * @return: 返回响应数据长度
 */
static int do_read_capacity_10(struct scsi_cmnd *cmd)
{
    u32 last_lba = VIRTUAL_DISK_SECTORS - 1;  /* 最后一个逻辑块地址（32位） */
    u32 sector_size = VIRTUAL_SECTOR_SIZE;    /* 扇区大小 */
    unsigned char buf[8]; /* 8字节响应缓冲区 */
    
    /* READ CAPACITY(10)响应格式 - 大端序 */
    buf[0] = (last_lba >> 24) & 0xFF; /* LBA高字节 */
    buf[1] = (last_lba >> 16) & 0xFF;
    buf[2] = (last_lba >> 8) & 0xFF;
    buf[3] = last_lba & 0xFF;         /* LBA低字节 */
    
    buf[4] = (sector_size >> 24) & 0xFF; /* 扇区大小高字节 */
    buf[5] = (sector_size >> 16) & 0xFF;
    buf[6] = (sector_size >> 8) & 0xFF;
    buf[7] = sector_size & 0xFF;         /* 扇区大小低字节 */
    
    cmd->sc_data_direction = DMA_FROM_DEVICE; /* 设置数据传输方向 */
    
    /* 将数据复制到scatter-gather列表（DMA_FROM_DEVICE: 从buf到sg） */
    scsi_sg_copy_from_buffer(cmd, buf, 8);
    
    cmd->result = DID_OK << 16; /* 设置命令结果为成功 */
    scsi_set_resid(cmd, 0); /* 没有剩余数据 */
    
    return 8; /* 返回8字节 */
}

/*
 * 执行SCSI READ CAPACITY(16)命令
 * @param cmd: SCSI命令结构体
 * @return: 返回响应数据长度
 */
static int do_read_capacity_16(struct scsi_cmnd *cmd)
{
    u64 last_lba = VIRTUAL_DISK_SECTORS - 1;  /* 最后一个逻辑块地址（64位） */
    u32 sector_size = VIRTUAL_SECTOR_SIZE;    /* 扇区大小 */
    unsigned char buf[32]; /* 32字节响应缓冲区 */
    
    memset(buf, 0, 32); /* 清零缓冲区 */
    
    /* READ CAPACITY(16)响应格式 - 大端序 */
    /* 字节0-7: 最后一个可访问块的逻辑块地址 */
    buf[0] = (last_lba >> 56) & 0xFF; /* LBA最高字节 */
    buf[1] = (last_lba >> 48) & 0xFF;
    buf[2] = (last_lba >> 40) & 0xFF;
    buf[3] = (last_lba >> 32) & 0xFF;
    buf[4] = (last_lba >> 24) & 0xFF;
    buf[5] = (last_lba >> 16) & 0xFF;
    buf[6] = (last_lba >> 8) & 0xFF;
    buf[7] = last_lba & 0xFF;         /* LBA最低字节 */
    
    /* 字节8-11: 逻辑块长度（字节） */
    buf[8] = (sector_size >> 24) & 0xFF;
    buf[9] = (sector_size >> 16) & 0xFF;
    buf[10] = (sector_size >> 8) & 0xFF;
    buf[11] = sector_size & 0xFF;
    
    cmd->sc_data_direction = DMA_FROM_DEVICE; /* 设置数据传输方向 */
    
    /* 将数据复制到scatter-gather列表（DMA_FROM_DEVICE: 从buf到sg） */
    scsi_sg_copy_from_buffer(cmd, buf, 32);
    
    cmd->result = DID_OK << 16; /* 设置命令结果为成功 */
    scsi_set_resid(cmd, 0); /* 没有剩余数据 */
    
    return 32; /* 返回32字节 */
}

/*
 * 辅助函数：从字节数组获取大端序32位值
 * @param p: 指向数据的指针
 * @return: 32位大端序值
 */
static inline u32 get_be32(const unsigned char *p)
{
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3];
}

/*
 * 辅助函数：从字节数组获取大端序16位值
 * @param p: 指向数据的指针
 * @return: 16位大端序值
 */
static inline u16 get_be16(const unsigned char *p)
{
    return ((u16)p[0] << 8) | p[1];
}

/*
 * 执行SCSI MODE SENSE(6)命令
 * 返回模式页面数据，包括缓存信息
 * @param cmd: SCSI命令结构体
 * @return: 返回响应数据长度
 */
static int do_mode_sense_6(struct scsi_cmnd *cmd)
{
    unsigned char *cdb = cmd->cmnd;  /* 获取命令描述块指针 */
    u32 alloc_len = cdb[4];          /* MODE SENSE(6)的alloc_len在cdb[4] */
    int dbd = (cdb[1] & 0x08) != 0;  /* DBD位：禁止块描述符 */
    int pcode = cdb[2] & 0x3F;       /* 页面代码 */
    int pcontrol = (cdb[2] >> 6) & 0x03;  /* 页面控制字段 */
    unsigned char buf[64]; /* 64字节响应缓冲区 */
    int offset = 0; /* 当前写入位置偏移 */
    int len; /* 实际响应长度 */
    
    memset(buf, 0, 64); /* 清零缓冲区 */
    
    /* 页面控制：0=当前值, 1=可更改值, 2=默认值, 3=保存值(不支持) */
    if (pcontrol == 3) {
        set_sense_data(cmd, ILLEGAL_REQUEST, 0x20, 0x00);
        cmd->result = (DID_OK << 16) | SAM_STAT_CHECK_CONDITION;
        return 0;
    }
    
    /* MODE SENSE(6)响应头 */
    buf[0] = 0;                      /* 模式数据长度（后面设置） */
    buf[1] = 0;                      /* 介质类型（0=无介质） */
    buf[2] = 0x10;                   /* 设备特定：DPOFUA=1 */
    buf[3] = dbd ? 0 : 8;            /* 块描述符长度 */
    offset = 4; /* 头4字节已填充 */
    
    /* 块描述符（8字节） */
    if (!dbd) {
        /* 块数量（大端序） */
        buf[offset++] = (VIRTUAL_DISK_SECTORS >> 24) & 0xFF;
        buf[offset++] = (VIRTUAL_DISK_SECTORS >> 16) & 0xFF;
        buf[offset++] = (VIRTUAL_DISK_SECTORS >> 8) & 0xFF;
        buf[offset++] = VIRTUAL_DISK_SECTORS & 0xFF;
        /* 保留 */
        buf[offset++] = 0;
        buf[offset++] = 0;
        /* 块长度（大端序） */
        buf[offset++] = (VIRTUAL_SECTOR_SIZE >> 8) & 0xFF;
        buf[offset++] = VIRTUAL_SECTOR_SIZE & 0xFF;
    }
    
    /* 处理特定页面代码 */
    switch (pcode) {
    case 0x00:  /* 所有页面 */
    case 0x08:  /* 缓存页面 */
        /* 缓存页面：页面代码=0x08, 页面长度=0x12(18字节) */
        buf[offset++] = 0x08;  /* 页面代码 */
        buf[offset++] = 0x12;  /* 页面长度(18) */
        buf[offset++] = 0x14;  /* 缓存控制：WCE=1, RCD=1 */
        buf[offset++] = 0x00;  /* 缓存大小(MSB) - 未使用 */
        buf[offset++] = 0xff;  /* 缓存大小 */
        buf[offset++] = 0xff;  /* 缓存大小 */
        buf[offset++] = 0x00;  /* 缓存大小(LSB) */
        buf[offset++] = 0x00;  /* 阈值 */
        buf[offset++] = 0xff;  /* 预取大小 */
        buf[offset++] = 0xff;  /* 预取大小 */
        buf[offset++] = 0xff;  /* 预取上限 */
        buf[offset++] = 0xff;  /* 预取上限 */
        buf[offset++] = 0x80;  /* 缓存禁用(CD)=0, FUA=1 */
        buf[offset++] = 0x14;  /* 缓存控制(与字节2相同) */
        buf[offset++] = 0x00;  /* 保留 */
        buf[offset++] = 0x00;  /* 保留 */
        buf[offset++] = 0x00;  /* 保留 */
        buf[offset++] = 0x00;  /* 保留 */
        buf[offset++] = 0x00;  /* 保留 */
        break;
    case 0x01:  /* 读写错误恢复页面 */
        /* 简单的错误恢复页面 */
        buf[offset++] = 0x01;  /* 页面代码 */
        buf[offset++] = 0x0a;  /* 页面长度(10) */
        buf[offset++] = 0x00;  /* 错误恢复控制 */
        buf[offset++] = 0x00;  /* 读重试次数 */
        buf[offset++] = 0x00;  /* 校正跨度 */
        buf[offset++] = 0x00;  /* 磁头偏移次数 */
        buf[offset++] = 0x00;  /* 数据选通偏移次数 */
        buf[offset++] = 0x00;  /* 写重试次数 */
        buf[offset++] = 0x00;  /* 恢复时间限制 */
        buf[offset++] = 0x00;  /* 恢复时间限制 */
        buf[offset++] = 0x00;  /* 保留 */
        break;
    default:
        /* 未知页面 - 只返回头 */
        break;
    }
    
    /* 设置总长度 */
    len = min(offset, (int)alloc_len);
    buf[0] = len - 1;  /* 模式数据长度 = 总长度 - 1 */
    
    cmd->sc_data_direction = DMA_FROM_DEVICE; /* 设置数据传输方向 */
    scsi_sg_copy_from_buffer(cmd, buf, len);  /* 将数据复制到sg列表 */
    cmd->result = DID_OK << 16;               /* 设置命令结果为成功 */
    scsi_set_resid(cmd, alloc_len - len);     /* 设置剩余长度 */
    
    return len; /* 返回实际传输长度 */
}

/*
 * 执行SCSI MODE SELECT(6)命令
 * @param cmd: SCSI命令结构体
 * @return: 返回0表示成功
 */
static int do_mode_select_6(struct scsi_cmnd *cmd)
{
    unsigned char *cdb = cmd->cmnd; /* 获取命令描述块指针 */
    u32 param_len = cdb[4]; /* MODE SELECT(6)的参数长度在cdb[4] */
    unsigned char *buf; /* 临时缓冲区指针 */
    
    /* 对于虚拟磁盘，接受但忽略模式选择 */
    cmd->sc_data_direction = DMA_TO_DEVICE; /* 设置数据传输方向 */
    
    /* 读取并丢弃数据 */
    buf = kmalloc(param_len, GFP_KERNEL); /* 分配临时缓冲区 */
    if (buf) {
        scsi_sg_copy_to_buffer(cmd, buf, param_len);     /* 从sg列表复制数据 */
        kfree(buf); /* 释放临时缓冲区 */
    }
    
    cmd->result = DID_OK << 16; /* 设置命令结果为成功 */
    scsi_set_resid(cmd, 0);     /* 没有剩余数据 */
    
    return 0;
}

/*
 * 执行SCSI MODE SENSE(10)命令
 * 返回模式页面数据，包括缓存信息
 * @param cmd: SCSI命令结构体
 * @return: 返回响应数据长度
 */
static int do_mode_sense_10(struct scsi_cmnd *cmd)
{
    unsigned char *cdb = cmd->cmnd; /* 获取命令描述块指针 */
    u32 alloc_len = get_unaligned_be16(cdb + 7); /* alloc_len是cdb[7:8]的2字节大端序值 */
    int dbd = (cdb[1] & 0x08) != 0;  /* DBD位：禁止块描述符 */
    int pcode = cdb[2] & 0x3F;       /* 页面代码 */
    int pcontrol = (cdb[1] >> 6) & 0x03;  /* 页面控制字段 */
    unsigned char buf[64]; /* 64字节响应缓冲区 */
    int offset = 0; /* 当前写入位置偏移 */
    int len; /* 实际响应长度 */
    
    memset(buf, 0, 64); /* 清零缓冲区 */
    
    /* 页面控制：0=当前值, 1=可更改值, 2=默认值, 3=保存值(不支持) */
    if (pcontrol == 3) {
        set_sense_data(cmd, ILLEGAL_REQUEST, 0x20, 0x00);
        cmd->result = (DID_OK << 16) | SAM_STAT_CHECK_CONDITION;
        return 0;
    }
    
    /* MODE SENSE(10)响应头 */
    buf[0] = 0;                      /* 模式数据长度高字节 */
    buf[1] = 0;                      /* 模式数据长度低字节 */
    buf[2] = 0;                      /* 介质类型（0=无介质） */
    buf[3] = 0x10;                   /* 设备特定：DPOFUA=1 */
    buf[7] = dbd ? 0 : 8;            /* 块描述符长度 */
    offset = 8; /* 头8字节已填充 */
    
    /* 块描述符（8字节） */
    if (!dbd) {
        /* 块数量（大端序） */
        buf[offset++] = (VIRTUAL_DISK_SECTORS >> 24) & 0xFF;
        buf[offset++] = (VIRTUAL_DISK_SECTORS >> 16) & 0xFF;
        buf[offset++] = (VIRTUAL_DISK_SECTORS >> 8) & 0xFF;
        buf[offset++] = VIRTUAL_DISK_SECTORS & 0xFF;
        /* 保留 */
        buf[offset++] = 0;
        buf[offset++] = 0;
        /* 块长度（大端序） */
        buf[offset++] = (VIRTUAL_SECTOR_SIZE >> 8) & 0xFF;
        buf[offset++] = VIRTUAL_SECTOR_SIZE & 0xFF;
    }
    
    /* 处理特定页面代码 */
    switch (pcode) {
    case 0x00:  /* 所有页面 */
    case 0x08:  /* 缓存页面 */
        /* 缓存页面：页面代码=0x08, 页面长度=0x12(18字节) */
        buf[offset++] = 0x08;  /* 页面代码 */
        buf[offset++] = 0x12;  /* 页面长度(18) */
        buf[offset++] = 0x14;  /* 缓存控制：WCE=1, RCD=1 */
        buf[offset++] = 0x00;  /* 缓存大小(MSB) - 未使用 */
        buf[offset++] = 0xff;  /* 缓存大小 */
        buf[offset++] = 0xff;  /* 缓存大小 */
        buf[offset++] = 0x00;  /* 缓存大小(LSB) */
        buf[offset++] = 0x00;  /* 阈值 */
        buf[offset++] = 0xff;  /* 预取大小 */
        buf[offset++] = 0xff;  /* 预取大小 */
        buf[offset++] = 0xff;  /* 预取上限 */
        buf[offset++] = 0xff;  /* 预取上限 */
        buf[offset++] = 0x80;  /* 缓存禁用(CD)=0, FUA=1 */
        buf[offset++] = 0x14;  /* 缓存控制(与字节2相同) */
        buf[offset++] = 0x00;  /* 保留 */
        buf[offset++] = 0x00;  /* 保留 */
        buf[offset++] = 0x00;  /* 保留 */
        buf[offset++] = 0x00;  /* 保留 */
        buf[offset++] = 0x00;  /* 保留 */
        break;
    case 0x01:  /* 读写错误恢复页面 */
        /* 简单的错误恢复页面 */
        buf[offset++] = 0x01;  /* 页面代码 */
        buf[offset++] = 0x0a;  /* 页面长度(10) */
        buf[offset++] = 0x00;  /* 错误恢复控制 */
        buf[offset++] = 0x00;  /* 读重试次数 */
        buf[offset++] = 0x00;  /* 校正跨度 */
        buf[offset++] = 0x00;  /* 磁头偏移次数 */
        buf[offset++] = 0x00;  /* 数据选通偏移次数 */
        buf[offset++] = 0x00;  /* 写重试次数 */
        buf[offset++] = 0x00;  /* 恢复时间限制 */
        buf[offset++] = 0x00;  /* 恢复时间限制 */
        buf[offset++] = 0x00;  /* 保留 */
        break;
    default:
        /* 未知页面 - 只返回头 */
        break;
    }
    
    /* 设置总长度 */
    len = min(offset, (int)alloc_len);
    put_unaligned_be16(len - 2, buf);  /* 模式数据长度 = 总长度 - 2 */
    
    cmd->sc_data_direction = DMA_FROM_DEVICE; /* 设置数据传输方向 */
    scsi_sg_copy_from_buffer(cmd, buf, len);  /* 将数据复制到sg列表 */
    cmd->result = DID_OK << 16; /* 设置命令结果为成功 */
    scsi_set_resid(cmd, alloc_len - len); /* 设置剩余长度 */
    
    return len; /* 返回实际传输长度 */
}

/*
 * 执行SCSI MODE SELECT(10)命令
 * @param cmd: SCSI命令结构体
 * @return: 返回0表示成功
 */
static int do_mode_select_10(struct scsi_cmnd *cmd)
{
    unsigned char *cdb = cmd->cmnd; /* 获取命令描述块指针 */
    u32 param_len = get_unaligned_be16(cdb + 7); /* 参数长度在cdb[7:8] */
    unsigned char *buf; /* 临时缓冲区指针 */
    
    /* 对于虚拟磁盘，接受但忽略模式选择 */
    cmd->sc_data_direction = DMA_TO_DEVICE; /* 设置数据传输方向 */
    
    /* 读取并丢弃数据 */
    buf = kmalloc(param_len, GFP_KERNEL); /* 分配临时缓冲区 */
    if (buf) {
        scsi_sg_copy_to_buffer(cmd, buf, param_len); /* 从sg列表复制数据 */
        kfree(buf); /* 释放临时缓冲区 */
    }
    
    cmd->result = DID_OK << 16; /* 设置命令结果为成功 */
    scsi_set_resid(cmd, 0); /* 没有剩余数据 */
    
    return 0;
}

/*
 * 执行SCSI READ命令（6字节或10字节）
 * @param cmd: SCSI命令结构体
 * @return: 返回读取的字节数或0表示失败
 */
static int do_read(struct scsi_cmnd *cmd)
{
    struct vhost_data *vhost = host_to_vhost(cmd->device->host); /* 获取虚拟主机数据 */
    unsigned char *cdb = cmd->cmnd; /* 获取命令描述块指针 */
    u32 lba; /* 逻辑块地址 */
    u32 num_sectors; /* 扇区数量 */
    unsigned long offset; /* 缓冲区偏移 */
    int ret = 0; /* 返回值 */
    
    if (cmd->cmd_len == 6) {
        /* READ(6)命令格式 */
        lba = ((cdb[1] & 0x1F) << 16) | (cdb[2] << 8) | cdb[3]; /* LBA在cdb[1:3] */
        num_sectors = cdb[4]; /* 扇区数在cdb[4] */
    } else {
        /* READ(10)命令格式 */
        lba = get_be32(&cdb[2]); /* LBA在cdb[2:5] */
        num_sectors = get_be16(&cdb[7]); /* 扇区数在cdb[7:8] */
    }
    
    /* 验证LBA和扇区数量 */
    if (lba + num_sectors > VIRTUAL_DISK_SECTORS) {
        set_sense_data(cmd, ILLEGAL_REQUEST, 0x21, 0x00); /* LBA越界 */
        cmd->result = (DID_OK << 16) | SAM_STAT_CHECK_CONDITION;
        return 0;
    }
    
    offset = lba * VIRTUAL_SECTOR_SIZE; /* 计算缓冲区偏移 */
    cmd->sc_data_direction = DMA_FROM_DEVICE; /* 设置数据传输方向 */
    
    spin_lock(&vhost->lock); /* 获取自旋锁 */
    
    /* 从虚拟磁盘复制数据到scatter-gather列表（DMA_FROM_DEVICE: 从buf到sg） */
    ret = scsi_sg_copy_from_buffer(cmd, vhost->disk_buffer + offset, 
                                   num_sectors * VIRTUAL_SECTOR_SIZE);
    
    spin_unlock(&vhost->lock); /* 释放自旋锁 */
    
    vhost->sectors_read += num_sectors; /* 更新读取统计 */
    
    if (ret < 0) {
        cmd->result = DID_ERROR << 16; /* 设置错误结果 */
        return 0;
    }
    
    cmd->result = DID_OK << 16; /* 设置成功结果 */
    scsi_set_resid(cmd, 0); /* 没有剩余数据 */
    return ret; /* 返回读取的字节数 */
}

/*
 * 执行SCSI WRITE命令（6字节或10字节）
 * @param cmd: SCSI命令结构体
 * @return: 返回写入的字节数或0表示失败
 */
static int do_write(struct scsi_cmnd *cmd)
{
    struct vhost_data *vhost = host_to_vhost(cmd->device->host); /* 获取虚拟主机数据 */
    unsigned char *cdb = cmd->cmnd; /* 获取命令描述块指针 */
    u32 lba; /* 逻辑块地址 */
    u32 num_sectors; /* 扇区数量 */
    unsigned long offset; /* 缓冲区偏移 */
    int ret = 0; /* 返回值 */
    
    if (cmd->cmd_len == 6) {
        /* WRITE(6)命令格式 */
        lba = ((cdb[1] & 0x1F) << 16) | (cdb[2] << 8) | cdb[3]; /* LBA在cdb[1:3] */
        num_sectors = cdb[4]; /* 扇区数在cdb[4] */
    } else {
        /* WRITE(10)命令格式 */
        lba = get_be32(&cdb[2]); /* LBA在cdb[2:5] */
        num_sectors = get_be16(&cdb[7]); /* 扇区数在cdb[7:8] */
    }
    
    /* 验证LBA和扇区数量 */
    if (lba + num_sectors > VIRTUAL_DISK_SECTORS) {
        set_sense_data(cmd, ILLEGAL_REQUEST, 0x21, 0x00); /* LBA越界 */
        cmd->result = (DID_OK << 16) | SAM_STAT_CHECK_CONDITION;
        return 0;
    }
    
    offset = lba * VIRTUAL_SECTOR_SIZE; /* 计算缓冲区偏移 */
    cmd->sc_data_direction = DMA_TO_DEVICE; /* 设置数据传输方向 */
    
    spin_lock(&vhost->lock); /* 获取自旋锁 */
    
    /* 从scatter-gather列表复制数据到虚拟磁盘（DMA_TO_DEVICE: 从sg到buf） */
    ret = scsi_sg_copy_to_buffer(cmd, vhost->disk_buffer + offset,
                                 num_sectors * VIRTUAL_SECTOR_SIZE);
    
    spin_unlock(&vhost->lock); /* 释放自旋锁 */
    
    vhost->sectors_written += num_sectors; /* 更新写入统计 */
    
    if (ret < 0) {
        cmd->result = DID_ERROR << 16; /* 设置错误结果 */
        return 0;
    }
    
    cmd->result = DID_OK << 16; /* 设置成功结果 */
    scsi_set_resid(cmd, 0); /* 没有剩余数据 */
    return ret; /* 返回写入的字节数 */
}

/*
 * 执行SCSI VERIFY命令
 * @param cmd: SCSI命令结构体
 * @return: 返回0表示成功
 */
static int do_verify(struct scsi_cmnd *cmd)
{
    unsigned char *cdb = cmd->cmnd; /* 获取命令描述块指针 */
    u32 lba = get_be32(&cdb[2]); /* LBA在cdb[2:5] */
    u32 num_sectors = get_be16(&cdb[7]); /* 扇区数在cdb[7:8] */
    
    /* 验证LBA和扇区数量 */
    if (lba + num_sectors > VIRTUAL_DISK_SECTORS) {
        set_sense_data(cmd, ILLEGAL_REQUEST, 0x21, 0x00); /* LBA越界 */
        cmd->result = (DID_OK << 16) | SAM_STAT_CHECK_CONDITION;
        return 0;
    }
    
    cmd->result = DID_OK << 16; /* 设置成功结果 */
    return 0;
}

/*
 * 执行SCSI SYNCHRONIZE CACHE命令
 * @param cmd: SCSI命令结构体
 * @return: 返回0表示成功
 */
static int do_synchronize_cache(struct scsi_cmnd *cmd)
{
    /* 虚拟磁盘不需要同步缓存（没有真实缓存） */
    cmd->result = DID_OK << 16; /* 设置成功结果 */
    return 0;
}

/*
 * 处理SCSI命令 - 从工作队列调用
 * @param work: 工作结构体指针
 */
static void vhost_process_command(struct work_struct *work)
{
    struct vhost_work *vwork = container_of(work, struct vhost_work, work); /* 获取工作项 */
    struct scsi_cmnd *cmd = vwork->cmd;  /* 获取SCSI命令 */
    struct Scsi_Host *host = cmd->device->host; /* 获取SCSI主机 */
    unsigned char opcode = cmd->cmnd[0]; /* 获取命令操作码 */
    int ret = 0; /* 返回值 */
    
    /* 只处理目标ID为0和LUN为0的命令 */
    if (cmd->device->id != 0 || cmd->device->lun != 0) {
        set_sense_data(cmd, NO_SENSE, 0x00, 0x00); /* 设置无错误 */
        cmd->result = (DID_NO_CONNECT << 16) | SAM_STAT_CHECK_CONDITION; /* 无连接 */
        vhost_scsi_done(cmd); /* 完成命令 */
        kfree(vwork); /* 释放工作项 */
        return;
    }
    
    /* 根据操作码分发处理 */
    switch (opcode) {
    case OPCODE_TEST_UNIT_READY:       /* 0x00 - 设备就绪检测(6字节)：检查设备是否可访问 */
        ret = do_test_unit_ready(cmd);
        break;
    case OPCODE_INQUIRY:               /* 0x12 - 查询设备信息(6字节)：返回厂商名、产品名等 */
        ret = do_inquiry(cmd);
        break;
    case OPCODE_MODE_SENSE_6:          /* 0x1A - 模式读取(6字节)：返回设备模式页面（缓存、错误恢复等） */
        ret = do_mode_sense_6(cmd);
        break;
    case OPCODE_MODE_SENSE_10:         /* 0x5A - 模式读取(10字节)：同上，支持更大的数据长度 */
        ret = do_mode_sense_10(cmd);
        break;
    case OPCODE_MODE_SELECT_6:         /* 0x15 - 模式配置(6字节)：设置设备模式参数 */
        ret = do_mode_select_6(cmd);
        break;
    case OPCODE_MODE_SELECT_10:        /* 0x55 - 模式配置(10字节)：同上，支持更大的数据长度 */
        ret = do_mode_select_10(cmd);
        break;
    case OPCODE_READ_CAPACITY_10:      /* 0x25 - 读取容量(10字节)：返回最大LBA和扇区大小（32位） */
        ret = do_read_capacity_10(cmd);
        break;
    case OPCODE_READ_CAPACITY_16:      /* 0x9E - 读取容量(16字节)：返回最大LBA和扇区大小（64位） */
        ret = do_read_capacity_16(cmd);
        break;
    case OPCODE_READ_6:                /* 0x08 - 读命令(6字节)：从磁盘读取数据（LBA < 2^24） */
    case OPCODE_READ_10:               /* 0x28 - 读命令(10字节)：从磁盘读取数据（LBA < 2^32） */
        ret = do_read(cmd);
        break;
    case OPCODE_WRITE_6:               /* 0x0A - 写命令(6字节)：向磁盘写入数据（LBA < 2^24） */
    case OPCODE_WRITE_10:              /* 0x2A - 写命令(10字节)：向磁盘写入数据（LBA < 2^32） */
        ret = do_write(cmd);
        break;
    case OPCODE_VERIFY_10:             /* 0x2F - 数据校验(10字节)：验证指定扇区的数据完整性 */
        ret = do_verify(cmd);
        break;
    case OPCODE_SYNCHRONIZE_CACHE:     /* 0x35 - 同步缓存(10字节)：将缓存数据刷新到磁盘 */
        ret = do_synchronize_cache(cmd);
        break;
    default:                           /* 未知命令：返回非法请求错误 */
        dev_dbg(&host->shost_gendev, "Unsupported SCSI command: 0x%02x\n", opcode);
        set_sense_data(cmd, ILLEGAL_REQUEST, 0x20, 0x00); /* 无效命令操作码 */
        cmd->result = (DID_OK << 16) | SAM_STAT_CHECK_CONDITION;
        ret = 0;
        break;
    }
    
    /* 异步完成命令 */
    vhost_scsi_done(cmd);
    
    /* 释放工作项 */
    kfree(vwork);
}

/*
 * 队列命令处理函数 - 所有SCSI命令在这里接收
 * @param host: SCSI主机
 * @param cmd: SCSI命令
 * @return: 返回0表示成功
 */
static int vhost_queuecommand(struct Scsi_Host *host, struct scsi_cmnd *cmd)
{
    struct vhost_data *vhost = host_to_vhost(host); /* 获取虚拟主机数据 */
    struct vhost_work *vwork; /* 工作项 */
    
    dev_dbg(&host->shost_gendev, "Queued SCSI command: opcode 0x%02x, cmd_len=%d\n", 
            cmd->cmnd[0], cmd->cmd_len);
    
    /* 为异步处理分配工作项 */
    vwork = kmalloc(sizeof(struct vhost_work), GFP_ATOMIC);
    if (!vwork) {
        dev_err(&host->shost_gendev, "Failed to allocate work item\n");
        cmd->result = DID_ERROR << 16; /* 设置错误结果 */
        vhost_scsi_done(cmd); /* 完成命令 */
        return -ENOMEM; /* 返回内存不足错误 */
    }
    
    /* 初始化工作结构体 */
    vwork->cmd = cmd; /* 关联命令 */
    INIT_WORK(&vwork->work, vhost_process_command); /* 初始化异步任务的执行回调函数 */
    
    /* 将工作项加入工作队列 */
    queue_work(vhost->wq, &vwork->work);
    
    return 0; /* 返回成功 */
}

/*
 * 中止处理函数 - 当命令需要中止时调用
 * @param cmd: 需要中止的SCSI命令
 * @return: 返回SUCCESS表示成功
 */
static int vhost_eh_abort_handler(struct scsi_cmnd *cmd)
{
    dev_info(&cmd->device->host->shost_gendev, "Abort command: opcode 0x%02x\n", 
             cmd->cmnd[0]);
    
    /* 对于虚拟驱动，直接完成命令并返回错误 */
    cmd->result = DID_ABORT << 16; /* 设置中止结果 */
    vhost_scsi_done(cmd); /* 完成命令 */
    
    return SUCCESS; /* 返回成功 */
}

/*
 * 主机重置处理函数 - 当主机需要重置时调用
 * @param cmd: SCSI命令，用于获取主机信息
 * @return: 返回SUCCESS表示成功
 */
static int vhost_eh_host_reset_handler(struct scsi_cmnd *cmd)
{
    struct vhost_data *vhost = host_to_vhost(cmd->device->host); /* 获取虚拟主机数据 */
    
    dev_info(&cmd->device->host->shost_gendev, "Host reset requested\n");
    
    /* 重置统计信息 */
    spin_lock(&vhost->lock); /* 获取自旋锁 */
    vhost->sectors_read = 0; /* 清零读取统计 */
    vhost->sectors_written = 0; /* 清零写入统计 */
    spin_unlock(&vhost->lock); /* 释放自旋锁 */
    
    return SUCCESS; /* 返回成功 */
}

/* SCSI主机指针，用于清理 */
static struct Scsi_Host *g_host;

/*
 * SCSI主机模板 - 定义主机适配器能力
 */
static struct scsi_host_template vhost_template = {
    .name = DRV_NAME,                                     /* 主机名称 */
    .module = THIS_MODULE,                                /* 所属模块 */
    .queuecommand = vhost_queuecommand,                   /* 队列命令处理函数 */
    .eh_abort_handler = vhost_eh_abort_handler,           /* 中止处理函数 */
    .eh_host_reset_handler = vhost_eh_host_reset_handler, /* 主机重置处理函数 */
    
    /* 主机能力 */
    .can_queue = 16,          /* 最大可队列命令数 */
    .this_id = 7,             /* 主机适配器的SCSI ID */
    .cmd_per_lun = 1,         /* 每个LUN的命令数 */
    .max_sectors = 256,       /* 每命令最大扇区数 */
    .sg_tablesize = 64,       /* 最大scatter-gather元素数 */
    .emulated = 1,            /* 这是一个模拟设备 */
};

/* 模块初始化函数 */
static int __init virtual_scsi_host_init(void)
{
    struct Scsi_Host *host; /* SCSI主机指针 */
    struct vhost_data *vhost; /* 虚拟主机数据指针 */
    int ret = 0; /* 返回值 */
    
    pr_info("%s: Virtual SCSI Host Driver v%s initializing...\n", DRV_NAME, DRV_VERSION);
    
    /* 分配SCSI主机，附带私有数据空间 */
    host = scsi_host_alloc(&vhost_template, sizeof(struct vhost_data));
    if (!host) {
        pr_err("%s: Failed to allocate SCSI host\n", DRV_NAME);
        return -ENOMEM; /* 返回内存不足错误 */
    }
    
    /* 获取私有数据区域指针 */
    vhost = shost_priv(host);
    
    /* 初始化自旋锁 */
    spin_lock_init(&vhost->lock);
    
    /* 使用vzalloc分配虚拟磁盘缓冲区，用于大内存分配 */ 
    vhost->disk_buffer = vzalloc(VIRTUAL_DISK_SIZE_MB * 1024 * 1024);
    if (!vhost->disk_buffer) {
        pr_err("%s: Failed to allocate virtual disk buffer (%d MB)\n", 
               DRV_NAME, VIRTUAL_DISK_SIZE_MB);
        ret = -ENOMEM;  /* 返回内存不足错误 */
        goto free_host; /* 跳转到清理 */
    }

    /* 向虚拟磁盘写入一些初始数据 */
    {
        unsigned int i; /* 循环变量 */
        for (i = 0; i < VIRTUAL_DISK_SECTORS; i++) {
            unsigned char *sector = vhost->disk_buffer + i * VIRTUAL_SECTOR_SIZE; /* 扇区指针 */
            memset(sector, (i & 0xFF), VIRTUAL_SECTOR_SIZE); /* 用扇区号的低8位填充 */
        }
    }
    
    pr_info("%s: Allocated virtual disk: %d MB (%u sectors, %d bytes/sector)\n",
            DRV_NAME, VIRTUAL_DISK_SIZE_MB, VIRTUAL_DISK_SECTORS, VIRTUAL_SECTOR_SIZE);
    
    /* 创建工作队列用于异步命令处理 */
    vhost->wq = create_singlethread_workqueue(DRV_NAME);
    if (!vhost->wq) {
        pr_err("%s: Failed to create workqueue\n", DRV_NAME);
        ret = -ENOMEM;    /* 返回内存不足错误 */
        goto free_buffer; /* 跳转到清理 */
    }
    
    /* 设置DMA掩码 */
    dma_set_mask(&host->shost_gendev, DMA_BIT_MASK(32));
    
    /* 将主机添加到系统 */
    ret = scsi_add_host(host, NULL);
    if (ret) {
        pr_err("%s: Failed to add SCSI host (ret=%d)\n", DRV_NAME, ret);
        goto free_wq; /* 跳转到清理 */
    }
    
    /* 扫描主机查找设备 */
    scsi_scan_host(host);
    
    /* 保存指针用于清理 */
    g_host = host;
    
    pr_info("%s: Virtual SCSI Host initialized successfully!\n", DRV_NAME);
    pr_info("%s: Statistics: sectors_read=%u, sectors_written=%u\n", 
            DRV_NAME, vhost->sectors_read, vhost->sectors_written);
    
    return 0;

free_wq:
    destroy_workqueue(vhost->wq); /* 销毁工作队列 */
free_buffer:
    vfree(vhost->disk_buffer); /* 释放虚拟磁盘缓冲区 */
free_host:
    scsi_host_put(host); /* 释放SCSI主机 */
    return ret;
}

/* 模块卸载函数 */
static void __exit virtual_scsi_host_exit(void)
{
    struct Scsi_Host *host = g_host; /* 获取SCSI主机指针 */
    struct vhost_data *vhost; /* 虚拟主机数据指针 */
    
    if (!host) {
        pr_err("%s: No SCSI host found\n", DRV_NAME);
        return; /* 无主机，直接返回 */
    }
    
    vhost = shost_priv(host); /* 获取私有数据 */
    
    pr_info("%s: Unloading Virtual SCSI Host Driver...\n", DRV_NAME);
    
    pr_info("%s: Final statistics:\n", DRV_NAME);
    pr_info("%s:   Sectors read:    %u\n", DRV_NAME, vhost->sectors_read);
    pr_info("%s:   Sectors written: %u\n", DRV_NAME, vhost->sectors_written);
    
    /* 移除主机 */
    scsi_remove_host(host);
    
    /* 等待所有待处理命令完成 */
    flush_workqueue(vhost->wq);
    
    /* 销毁工作队列 */
    destroy_workqueue(vhost->wq);
    
    /* 释放虚拟磁盘缓冲区 */
    vfree(vhost->disk_buffer);
    
    /* 释放主机引用 */
    scsi_host_put(host);
    
    /* 清空全局指针 */
    g_host = NULL;
    
    pr_info("%s: Virtual SCSI Host Driver unloaded successfully!\n", DRV_NAME);
}

module_init(virtual_scsi_host_init);
module_exit(virtual_scsi_host_exit);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("virtual_scsi_host module");
MODULE_LICENSE("GPL v2");