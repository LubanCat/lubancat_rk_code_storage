#include <linux/module.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cpumask.h>
#include <linux/vmalloc.h>
#include <linux/pci_regs.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>

#define DRV_NAME    "pcie_explorer"  /* 驱动名称 */
#define DRV_VERSION "1.0"            /* 驱动版本号 */

/* 设备私有数据结构体 */
struct pcie_dev {
    struct pci_dev *pdev;   /* PCI设备指针 */
    void __iomem *bar_mmap; /* BAR2映射地址 */
    size_t bar_size;        /* BAR2大小 */
    struct mutex lock;      /* 并发访问保护锁 */
};

/*  配置空间可写寄存器白名单，偏移查看pci_regs.h */
static const u32 config_whitelist[] = {
    0x04, 0x05,  /* PCI_COMMAND */
    0x0C, 0x0D,  /* PCI_CACHE_LINE_SIZE/PCI_LATENCY_TIMER */
    0x3C, 0x3D,  /* PCI_INTERRUPT_LINE/PCI_INTERRUPT_PIN */
};
#define CONFIG_WHITELIST_SIZE ARRAY_SIZE(config_whitelist)

/* BAR空间可写寄存器白名单，参考rtw88/pci.h */
static const u32 bar_whitelist[] = {
    0x00B4,  /* RTK_PCI_HISR0 - 中断状态0 */
    0x00BC,  /* RTK_PCI_HISR1 - 中断状态1 */
    0x0300,  /* RTK_PCI_CTRL - DMA控制 */
};
#define BAR_WHITELIST_SIZE ARRAY_SIZE(bar_whitelist)

/* 检查偏移是否在白名单中 */
static bool is_config_writable(u32 offset, u8 size)
{
    int i, j;
    /* 检查写入的每一个字节是否都在白名单中 */
    for (j = 0; j < size; j++) {
        bool found = false;
        for (i = 0; i < CONFIG_WHITELIST_SIZE; i++) {
            if (config_whitelist[i] == offset + j) {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }
    return true;
}

/* 检查BAR偏移是否在白名单中 */
static bool is_bar_writable(u32 offset)
{
    int i;
    for (i = 0; i < BAR_WHITELIST_SIZE; i++) {
        if (bar_whitelist[i] == offset)
            return true;
    }
    return false;
}

/* Capability名称查找表  */
static const char *get_cap_name(u8 cap_id)
{   
    /* 根据Capability ID返回名称 */
    switch (cap_id) {
    case PCI_CAP_ID_PM:     return "Power Management";    /* 电源管理 */
    case PCI_CAP_ID_AGP:    return "AGP";                 /* 加速图形端口 */
    case PCI_CAP_ID_VPD:    return "VPD";                 /* 关键产品数据 */
    case PCI_CAP_ID_SLOTID: return "Slot Identification"; /* 插槽标识 */
    case PCI_CAP_ID_MSI:    return "MSI";                 /* 消息信号中断 */
    case PCI_CAP_ID_MSIX:   return "MSI-X";               /* 扩展消息信号中断 */
    case PCI_CAP_ID_EXP:    return "PCI Express";         /* PCI Express */
    case PCI_CAP_ID_SSVID:  return "Subsystem Vendor ID"; /* 子系统厂商ID */
    default:                return "Unknown";             /* 未知Capability */
    }
}

static const char *get_ext_cap_name(u16 cap_id)
{
    /* 根据扩展Capability ID返回名称 */
    switch (cap_id) {
    case PCI_EXT_CAP_ID_ERR:   return "Advanced Error Reporting";             /* 高级错误报告 */
    case PCI_EXT_CAP_ID_VC:    return "Virtual Channel";                      /* 虚拟通道 */
    case PCI_EXT_CAP_ID_DSN:   return "Device Serial Number";                 /* 设备序列号 */
    case PCI_EXT_CAP_ID_PWR:   return "Power Budgeting";                      /* 功率预算 */
    case PCI_EXT_CAP_ID_RCLD:  return "Root Complex Link Declaration";        /* 根复合体链路声明 */
    case PCI_EXT_CAP_ID_RCEC:  return "Root Complex Event Collector";         /* 根复合体事件收集器 */
    case PCI_EXT_CAP_ID_VNDR:  return "Vendor Specific";                      /* 厂商特定 */
    case PCI_EXT_CAP_ID_L1SS:  return "L1 PM Substates";                      /* L1电源管理子状态 */
    case PCI_EXT_CAP_ID_SRIOV: return "Single Root I/O Virtualization";       /* 单根IO虚拟化 */
    case PCI_EXT_CAP_ID_PRI:   return "Page Request Interface";               /* 页面请求接口 */
    case PCI_EXT_CAP_ID_ATS:   return "Address Translation Services";         /* 地址转换服务 */
    case PCI_EXT_CAP_ID_PTM:   return "Precision Time Measurement";           /* 精确时间测量 */
    default:                   return "Unknown";                              /* 未知扩展Capability */
    }
}

/*  配置空间解析 */
static void print_config_space(struct pci_dev *pdev)
{   
    /* 配置空间标准寄存器 */
    u16 vendor_id, device_id, command, status;
    /* 配置空间字节寄存器 */
    u8  revision, header_type, interrupt_line, interrupt_pin;
    /* 延迟定时器和缓存行大小 */
    u8  lat_timer, cache_line_sz;
    /* 类别代码和Capability指针 */
    u32 class_code, cis_ptr;
    /* 子系统厂商ID和设备ID */
    u16 subsys_vendor, subsys_id;

    /* 读取厂商ID */
    pci_read_config_word(pdev, PCI_VENDOR_ID, &vendor_id);
    /* 读取设备ID */
    pci_read_config_word(pdev, PCI_DEVICE_ID, &device_id);
    /* 读取命令寄存器 */
    pci_read_config_word(pdev, PCI_COMMAND, &command);
    /* 读取状态寄存器 */
    pci_read_config_word(pdev, PCI_STATUS, &status);
    /* 读取修订版本 */
    pci_read_config_byte(pdev, PCI_REVISION_ID, &revision);
    /* 读取类别代码 */
    pci_read_config_dword(pdev, PCI_CLASS_REVISION, &class_code);
    /* 读取缓存行大小 */
    pci_read_config_byte(pdev, PCI_CACHE_LINE_SIZE, &cache_line_sz);
    /* 读取延迟定时器 */
    pci_read_config_byte(pdev, PCI_LATENCY_TIMER, &lat_timer);
    /* 读取头类型 */
    pci_read_config_byte(pdev, PCI_HEADER_TYPE, &header_type);
    /* 读取中断线 */
    pci_read_config_byte(pdev, PCI_INTERRUPT_LINE, &interrupt_line);
    /* 读取中断引脚 */
    pci_read_config_byte(pdev, PCI_INTERRUPT_PIN, &interrupt_pin);
    /* 读取子系统厂商ID */
    pci_read_config_word(pdev, PCI_SUBSYSTEM_VENDOR_ID, &subsys_vendor);
    /* 读取子系统设备ID */
    pci_read_config_word(pdev, PCI_SUBSYSTEM_ID, &subsys_id);
    /* 读取Capability链表指针 */
    pci_read_config_dword(pdev, PCI_CAPABILITY_LIST, &cis_ptr);
    
    pr_info("      PCIe 配置空间      \n");                               /* 打印标题 */
    pr_info("  Vendor: 0x%04X  Device: 0x%04X\n", vendor_id, device_id);  /* 打印厂商和设备ID */
    pr_info("  Revision: 0x%02X  Header: 0x%02X %s\n",                    /* 打印修订版本和头类型 */
        revision, header_type,
        (header_type & 0x80) ? "Multi-function" : "Single-function");  /* 判断单功能/多功能 */
    pr_info("  Class: 0x%08X (IF=0x%02X Sub=0x%02X Base=0x%02X)\n",    /* 打印类别代码分解 */
        class_code,
        (class_code >> 8) & 0xFF,   /* 接口级编程接口 */
        (class_code >> 16) & 0xFF,   /* 子类 */
        (class_code >> 24) & 0xFF); /* 基类 */
    pr_info("  Command: 0x%04X [%s%s%s%s%s]\n",         /* 打印命令寄存器位域 */
        command,
        (command & PCI_COMMAND_IO) ? "IO " : "",      /* IO 空间访问使能 */
        (command & PCI_COMMAND_MEMORY) ? "MEM " : "", /* 内存空间访问使能 */
        (command & PCI_COMMAND_MASTER) ? "BM " : "",  /* 总线主控使能 */
        (command & PCI_COMMAND_PARITY) ? "PAR " : "", /* 奇偶校验使能 */
        (command & PCI_COMMAND_SERR) ? "SERR" : "");  /* 系统错误使能 */
    pr_info("  Status: 0x%04X\n", status);            /* 打印状态寄存器 */
    pr_info("  Subsys: Vendor=0x%04X Device=0x%04X\n", subsys_vendor, subsys_id); /* 打印子系统信息 */
    pr_info("  IRQ: Line=%d Pin=%d\n", interrupt_line, interrupt_pin);            /* 打印中断信息 */
    pr_info("  Cache: %d bytes  Latency: %d\n", cache_line_sz * 4, lat_timer);    /* 打印缓存和延迟信息 */
    pr_info("  Capability PTR: 0x%02X\n", (u8)cis_ptr);                           /* 打印Capability指针 */
    pr_info("                 \n");
}

/* 解析标准Capability */
static void parse_capabilities(struct pci_dev *pdev)
{
    /* Capability ID 列表 */
    static const u8 cap_ids[] = {
        PCI_CAP_ID_PM,
        PCI_CAP_ID_MSI,
        PCI_CAP_ID_MSIX,
        PCI_CAP_ID_EXP,
        PCI_CAP_ID_VPD,
        PCI_CAP_ID_SSVID,
        PCI_CAP_ID_AGP,
        PCI_CAP_ID_SLOTID,
    };
    /* 循环变量 */
    int i, count = 0;

    /* 打印标题 */
    pr_info("      Capability 链表      \n");

    /* 遍历所有已知的Capability ID */
    for (i = 0; i < ARRAY_SIZE(cap_ids); i++) {
        int offset = pci_find_capability(pdev, cap_ids[i]);
        if (offset == 0) 
            continue;

        /* 打印Capability信息 */
        pr_info("  [%d] @0x%02X: ID=0x%02X (%s)\n",
            count, offset, cap_ids[i], get_cap_name(cap_ids[i]));

        /* 根据Capability ID解析详细信息 */
        switch (cap_ids[i]) {
        /* 电源管理Capability */
        case PCI_CAP_ID_PM: {
            /* 电源管理控制寄存器 */
            u16 pm_ctrl;
            /* 读取PM控制寄存器 */
            pci_read_config_word(pdev, offset + PCI_PM_CTRL, &pm_ctrl);
            /* 打印PM状态信息 */
            pr_info("      PM: State=%d, D1=%s, D2=%s, PME=%s, PMEStatus=%s\n",
                pm_ctrl & PCI_PM_CTRL_STATE_MASK,                   /* 当前电源状态 */
                (pm_ctrl & PCI_PM_CAP_D1) ? "Yes" : "No",           /* D1状态支持 */
                (pm_ctrl & PCI_PM_CAP_D2) ? "Yes" : "No",           /* D2状态支持 */
                (pm_ctrl & PCI_PM_CAP_PME) ? "Yes" : "No",          /* PME状态支持 */
                (pm_ctrl & PCI_PM_CTRL_PME_STATUS) ? "Yes" : "No"); /* PME状态 */
            break;
        }
        /* MSI Capability */
        case PCI_CAP_ID_MSI: {
            /* MSI控制寄存器 */
            u16 msg_ctrl;
            /* MSI地址低32位 */
            u32 msg_addr_lo;
            /* MSI数据 */
            u16 msg_data;
            /* 读取MSI控制寄存器 */
            pci_read_config_word(pdev, offset + PCI_MSI_FLAGS, &msg_ctrl);
            /* 读取MSI地址 */
            pci_read_config_dword(pdev, offset + PCI_MSI_ADDRESS_LO, &msg_addr_lo);
            /* 读取MSI数据 */
            pci_read_config_word(pdev, offset + PCI_MSI_DATA_32, &msg_data);
            /* 打印MSI信息 */
            pr_info("      MSI: Enable=%s, Addr64=%s, Data=0x%04X\n",
                (msg_ctrl & PCI_MSI_FLAGS_ENABLE) ? "Yes" : "No",      /* MSI启用状态 */
                (msg_ctrl & PCI_MSI_FLAGS_64BIT) ? "Yes" : "No",       /* 64位地址支持 */
                msg_data);                                             /* MSI数据值 */
            break;
        }
        /* MSI-X Capability */
        case PCI_CAP_ID_MSIX: {
            /* MSI-X控制寄存器 */
            u16 msg_ctrl;
            /* MSI-X表和PBA偏移 */
            u32 table_off, pba_off;
            /* 读取MSI-X控制寄存器 */
            pci_read_config_word(pdev, offset + PCI_MSIX_FLAGS, &msg_ctrl);
            /* 读取MSI-X表偏移 */
            pci_read_config_dword(pdev, offset + PCI_MSIX_TABLE, &table_off);
            /* 读取PBA偏移 */
            pci_read_config_dword(pdev, offset + PCI_MSIX_PBA, &pba_off);
            /* 打印MSI-X信息 */
            pr_info("      MSI-X: Enable=%s, TotalVec=%d\n",
                (msg_ctrl & PCI_MSIX_FLAGS_ENABLE) ? "Yes" : "No",  /* MSI-X启用状态 */
                msg_ctrl & PCI_MSIX_FLAGS_QSIZE);                   /* MSI-X向量数量 */
            break;
        }
        /* PCI Express Capability */
        case PCI_CAP_ID_EXP: {
            /* 链路能力是32位，链路状态是16位 */
            u32 lnk_cap;
            u16 lnk_status;
            /* 解析出的速度和宽度 */
            u8 max_speed, max_width, cur_speed, cur_width;
            /* 速度字符串映射 */
            const char *max_spd_str, *cur_spd_str;

            /* 读取链路能力 (Offset 0Ch, 32位寄存器, 4字节对齐) */
            pci_read_config_dword(pdev, offset + PCI_EXP_LNKCAP, &lnk_cap);
            
            /* 
            * 读取链路状态 (Offset 12h, 16位寄存器)
            * 必须使用pci_read_config_word！因为0x12不是4字节对齐，
            * 使用dword读取会导致硬件对齐错误，直接返回0！
            */
            pci_read_config_word(pdev, offset + PCI_EXP_LNKSTA, &lnk_status);

            /* 调试打印，验证读取结果 */
            pr_info("      DEBUG: LNKCAP=0x%08X, LNKSTA=0x%04X\n", lnk_cap, lnk_status);

            /* 解析能力寄存器 (Bit 0-3: 速度, Bit 4-9: 宽度) */
            max_speed = lnk_cap & 0x0F;
            max_width = (lnk_cap >> 4) & 0x3F;

            /* 解析状态寄存器 (Bit 0-3: 速度, Bit 4-9: 宽度) */
            cur_speed = lnk_status & 0x0F;
            cur_width = (lnk_status >> 4) & 0x3F;

            /* 映射最大链路速度 (1=2.5, 2=5.0, 3=8.0) */
            max_spd_str = (max_speed == 1) ? "2.5" :
                        (max_speed == 2) ? "5.0" :
                        (max_speed == 3) ? "8.0" : "?";
            pr_info("      PCIe: MaxLanes=x%d, MaxSpeed=%sGT/s\n",    /* 打印最大链路宽度和速度 */
                max_width, max_spd_str);                              /* 解析能力寄存器 */

            /* 映射协商链路速度 (1=2.5, 2=5.0, 3=8.0) */
            cur_spd_str = (cur_speed == 1) ? "2.5" :
                        (cur_speed == 2) ? "5.0" :
                        (cur_speed == 3) ? "8.0" : "?";
            pr_info("            NegLanes=x%d, NegSpeed=%sGT/s\n",    /* 打印协商链路宽度和速度 */
                cur_width, cur_spd_str);                              /* 解析状态寄存器 */
            break;
        }
        default:
            break;
        }
        count++;
    }

    pr_info("  共 %d 个 Capability\n", count);
    pr_info("                       \n");
}

/* 解析扩展Capability */
static void parse_extended_capabilities(struct pci_dev *pdev)
{
    /* 扩展Capability ID 列表 */
    static const u16 ext_cap_ids[] = {
        PCI_EXT_CAP_ID_ERR,
        PCI_EXT_CAP_ID_VC,
        PCI_EXT_CAP_ID_DSN,
        PCI_EXT_CAP_ID_PWR,
        PCI_EXT_CAP_ID_RCLD,
        PCI_EXT_CAP_ID_RCEC,
        PCI_EXT_CAP_ID_VNDR,
        PCI_EXT_CAP_ID_L1SS,
        PCI_EXT_CAP_ID_SRIOV,
        PCI_EXT_CAP_ID_PRI,
        PCI_EXT_CAP_ID_ATS,
        PCI_EXT_CAP_ID_PTM,
    };
    /* 循环变量 */
    int i, count = 0;

    /* 打印标题 */
    pr_info("      Extended Capability      \n");

    /* 遍历所有已知的扩展Capability ID */
    for (i = 0; i < ARRAY_SIZE(ext_cap_ids); i++) {
        int offset = pci_find_ext_capability(pdev, ext_cap_ids[i]);
        if (offset == 0)
            continue;

        /* 打印扩展Capability信息 */
        pr_info("  [%d] @0x%03X: ID=0x%04X (%s)\n",
            count, offset, ext_cap_ids[i], get_ext_cap_name(ext_cap_ids[i]));
        count++;
    }

    pr_info("  共 %d 个 Extended Capability\n", count);
    pr_info("                              \n");
}

/* 解析BAR信息 */
static void parse_bar_info(struct pcie_dev *edev)
{
    /* 获取PCI设备指针 */
    struct pci_dev *pdev = edev->pdev;
    /* BAR索引 */
    int i;

    /* 打印标题 */
    pr_info("      BAR 空间解析      \n");

    /* 遍历BAR0-BAR5 */
    for (i = 0; i < 6; i++) {
        /* BAR原始值 */
        u32 bar_raw;
        /* BAR基地址和大小 */
        u64 bar_base, bar_size;
        /* BAR属性标志 */
        bool is_mem, is_prefetch, is_64bit;

        /* 读取BAR寄存器 */
        pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0 + i * 4, &bar_raw);
        /* 判断是否为内存映射BAR */
        is_mem = !(bar_raw & 0x01);

        /* I/O映射BAR */
        if (!is_mem) {
            /* 提取I/O基地址 */
            bar_base = bar_raw & 0xFFFFFFFC;
             /* 获取BAR大小 */
            bar_size = pci_resource_len(pdev, i);
            /* 打印I/O BAR信息 */
            pr_info("  BAR%d: I/O  0x%08llX (size=%llu)\n",
                i, (unsigned long long)bar_base, (unsigned long long)bar_size);
        /* 内存映射 BAR */
        } else {
            /* 判断是否支持预取 */
            is_prefetch = (bar_raw >> 3) & 0x01;
            /* 判断是否为64位BAR */
            is_64bit = (bar_raw >> 2) & 0x01;
            /* 获取BAR基地址 */
            bar_base = pci_resource_start(pdev, i);
            /* 获取BAR大小 */
            bar_size = pci_resource_len(pdev, i);
            
            /* 打印内存BAR信息，动态选择单位，避免小于1MB时显示为0 */
            if (bar_size >= 1024 * 1024) {
                /* 大于等于1MB，以MB为单位 */
                pr_info("  BAR%d: MEM  0x%016llX (size=%llu MB)%s%s\n",
                    i, (unsigned long long)bar_base,
                    (unsigned long long)(bar_size / (1024 * 1024)),  /* 转换为MB */
                    is_64bit ? " 64bit" : "",         /* 64位标志 */
                    is_prefetch ? " Prefetch" : "");  /* 预取标志 */
            } else {
                /* 小于1MB，以KB为单位 */
                pr_info("  BAR%d: MEM  0x%016llX (size=%llu KB)%s%s\n",
                    i, (unsigned long long)bar_base,
                    (unsigned long long)(bar_size / 1024),           /* 转换为KB */
                    is_64bit ? " 64bit" : "",         /* 64位标志 */
                    is_prefetch ? " Prefetch" : "");  /* 预取标志 */
            }
        }

        /* 64位BAR占用两个BAR号 */
        if (is_64bit)
            i++;  /* 跳过下一个BAR */
    }

    pr_info("                        \n");
}

/* 映射BAR2并读取一些寄存器 */
static void map_and_explore_bar(struct pcie_dev *edev)
{
    /* 获取PCI设备指针 */
    struct pci_dev *pdev = edev->pdev;
    /* BAR起始地址和长度 */
    unsigned long bar_start, bar_len;
    /* 读取的寄存器值 */
    u32 val;
    /* 循环变量 */
    int i;

    /* 获取BAR2起始地址，通常BAR2是RTL网卡的寄存器空间 */
    bar_start = pci_resource_start(pdev, 2);
    /* 获取BAR2长度 */
    bar_len = pci_resource_len(pdev, 2);

    /* 检查BAR2是否有效 */
    if (bar_len == 0) {
        pr_err("BAR2 大小为 0\n");
        return;
    }

    /* 打印标题 */
    pr_info("      BAR2 MMIO 探索      \n");
    pr_info("  基地址: 0x%08lX, 大小: %lu bytes\n", bar_start, bar_len); /* 打印基地址和大小 */

    /* 映射BAR2到内核虚拟地址空间(非预取内存空间) */
    edev->bar_mmap = pci_iomap(pdev, 2, bar_len);
    if (!edev->bar_mmap) {
        pr_err("BAR2 映射失败\n");
        return;
    }
    /* 保存BAR2大小 */
    edev->bar_size = bar_len;

    pr_info("  MMIO 虚拟地址: %p\n", edev->bar_mmap);  /* 打印虚拟地址 */
    pr_info("  读取前64个字节的寄存器:\n");            /* 打印读取提示 */

    /* 读取BAR2前64字节，每次读4字节 */
    for (i = 0; i < 64; i += 4) {
        /* 读取32位寄存器 */
        val = readl(edev->bar_mmap + i);
        /* 每行打印4个寄存器 */
        if (i % 16 == 0)
            pr_info("    %04X: ", i);  /* 打印偏移地址 */
        pr_cont("%08X ", val);         /* 打印寄存器值，不换行 */
        /* 每行最后一个寄存器 */
        if (i % 16 == 12)
            pr_cont("\n");  /* 换行 */
    }
    pr_info("\n");
    pr_info("                         \n");
}

/* sysfs属性：显示配置空间信息 */
static ssize_t config_space_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    /* 转换为PCI设备 */
    struct pci_dev *pdev = to_pci_dev(dev);
    /* 偏移和缓冲区长度 */
    int pos, len = 0;
    /* 读取的32位值 */
    u32 val32;

    /* 打印标题 */
    len += sprintf(buf + len, "PCIe 配置空间 (前 256 字节):\n");
    /* 每次读4字节，共64次 */
    for (pos = 0; pos < 0x100; pos += 4) {
        pci_read_config_dword(pdev, pos, &val32);  /* 读取配置空间 */
        len += sprintf(buf + len, "0x%02X: 0x%08X\n", pos, val32); /* 格式化输出 */
    }

    /* 返回写入字节数 */
    return len;
}

/* sysfs属性：读取配置空间寄存器 */
static ssize_t config_read_store(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t count)
{
    /* 获取设备私有数据 */
    struct pcie_dev *edev = dev_get_drvdata(dev);
    /* 转换为PCI设备 */
    struct pci_dev *pdev = to_pci_dev(dev);
    /* 偏移和读取值 */
    u32 offset, val;
    /* 读取大小 */
    u8 size;
    int ret;

    /* 检查设备私有数据 */
    if (!edev)
        return -ENODEV;

    /* 获取锁 */
    ret = mutex_lock_interruptible(&edev->lock);
    if (ret)
        return ret;

    /* 解析用户输入 */
    if (sscanf(buf, "%x %hhu", &offset, &size) != 2) {
        pr_err("格式错误: 应为'offset size'\n");
        mutex_unlock(&edev->lock);
        return -EINVAL;
    }

    /* 检查参数有效性 */
    if (offset >= 0x100 || (size != 1 && size != 2 && size != 4)) {
        pr_err("无效参数: offset=0x%03X size=%d\n", offset, size);
        mutex_unlock(&edev->lock);
        return -EINVAL;
    }

    /* 根据大小读取配置空间 */
    switch (size) {
    /* 读取1字节 */
    case 1: {
        /* 8位值 */
        u8 val8;
        /* 读取1字节 */
        pci_read_config_byte(pdev, offset, &val8);
        /* 打印结果 */
        pr_info("CONFIG[0x%02X]: 0x%02X\n", offset, val8);
        break;
    }
    /* 读取2字节 */
    case 2: {
        /* 16位值 */
        u16 val16;
        /* 读取2字节 */
        pci_read_config_word(pdev, offset, &val16);
        /* 打印结果 */
        pr_info("CONFIG[0x%02X]: 0x%04X\n", offset, val16);
        break;
    }
    /* 读取4字节 */
    case 4:
        /* 读取4字节 */
        pci_read_config_dword(pdev, offset, &val);
        /* 打印结果 */
        pr_info("CONFIG[0x%02X]: 0x%08X\n", offset, val);
        break;
    }

    /* 释放锁 */
    mutex_unlock(&edev->lock);

    /* 返回读取字节数 */
    return count;
}

/* sysfs属性：写入配置空间寄存器 */
static ssize_t config_write_store(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t count)
{
    /* 获取设备私有数据 */
    struct pcie_dev *edev = dev_get_drvdata(dev);
    /* 转换为PCI设备 */
    struct pci_dev *pdev = to_pci_dev(dev);
    /* 偏移和写入值 */
    u32 offset, value;
    /* 写入大小 */
    u8 size;
    int ret;

    /* 检查设备私有数据 */
    if (!edev)
        return -ENODEV;

    /* 获取锁 */
    ret = mutex_lock_interruptible(&edev->lock);
    if (ret)
        return ret;

    /* 解析用户输入 */
    if (sscanf(buf, "%x %hhu %x", &offset, &size, &value) != 3) {
        pr_err("格式错误: 应为'offset size value'\n");
        mutex_unlock(&edev->lock);
        return -EINVAL;
    }

    /* 检查参数有效性 */
    if (offset >= 0x100 || (size != 1 && size != 2 && size != 4)) {
        pr_err("无效参数: offset=0x%03X size=%d\n", offset, size);
        mutex_unlock(&edev->lock);
        return -EINVAL;
    }

    /* 白名单检查，防止误操作 */
    if (!is_config_writable(offset, size)) {
        pr_err("寄存器 0x%02X (size=%d) 不在白名单中\n", offset, size);
        pr_info("允许写入的配置空间寄存器（绝对偏移）:\n");
        pr_info("  0x04, 0x05 - PCI_COMMAND 命令寄存器\n");
        pr_info("  0x0C, 0x0D - PCI_LATENCY_TIMER/PCI_CACHE_LINE_SIZE\n");
        pr_info("  0x3C, 0x3D - PCI_INTERRUPT_LINE/PIN\n");
        mutex_unlock(&edev->lock);
        return -EPERM;
    }

    /* 根据大小写入配置空间 */
    switch (size) {
    case 1:
        /* 写入1字节 */
        pci_write_config_byte(pdev, offset, (u8)value);
        break;
    case 2:
        /* 写入2字节 */
        pci_write_config_word(pdev, offset, (u16)value);
        break;  /* 跳出 switch */
    case 4:
        /* 写入4字节 */
        pci_write_config_dword(pdev, offset, value);
        break;
    }

    /* 打印写入结果 */
    pr_info("CONFIG_WRITE[0x%02X]: 0x%08X (size=%d)\n", offset, value, size);

    /* 释放锁 */
    mutex_unlock(&edev->lock);

    /* 返回写入字节数 */
    return count;
}

/* sysfs属性：显示BAR空间信息 */
static ssize_t bar_info_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    /* 转换为PCI设备 */
    struct pci_dev *pdev = to_pci_dev(dev);
    /* 获取设备私有数据 */
    struct pcie_dev *edev = dev_get_drvdata(dev);
    /* 循环变量和缓冲区长度 */
    int i, len = 0;

    /* 打印标题 */
    len += sprintf(buf + len, "BAR 空间信息:\n");

    /* 遍历BAR0-BAR5 */
    for (i = 0; i < 6; i++) {
        /* BAR起始地址和大小 */
        resource_size_t start, size;
        /* BAR标志 */
        unsigned long flags;

        /* 获取BAR起始地址 */
        start = pci_resource_start(pdev, i);
        /* 获取BAR大小 */
        size = pci_resource_len(pdev, i);
        /* 获取BAR标志 */
        flags = pci_resource_flags(pdev, i);

        /* 跳过空BAR */
        if (size == 0)
            continue;  /* 继续下一个 */
        
        /* 内存映射BAR */
        if (flags & IORESOURCE_MEM) {
            /* 动态选择单位，避免小于1MB时显示为0 */
            if (size >= 1024 * 1024) {
                /* 大于等于1MB，以MB为单位 */
                len += sprintf(buf + len,
                    "BAR%d: MEM 0x%016llX (size=%llu MB)\n",
                    i, (unsigned long long)start,
                    (unsigned long long)(size / (1024 * 1024)));
            } else {
                /* 小于1MB，以KB为单位 */
                len += sprintf(buf + len,
                    "BAR%d: MEM 0x%016llX (size=%llu KB)\n",
                    i, (unsigned long long)start,
                    (unsigned long long)(size / 1024));
            }
        }
        /* I/O映射BAR */
        else if (flags & IORESOURCE_IO)
            len += sprintf(buf + len,
                "BAR%d: I/O 0x%08llX\n",                     /* 打印I/O BAR信息 */
                i, (unsigned long long)start);
    }

    /* 检查BAR2是否已映射 */
    if (edev && edev->bar_mmap)
        len += sprintf(buf + len, "BAR2 已映射: %p (size=%zu)\n",  /* 打印BAR2映射信息 */
            edev->bar_mmap, edev->bar_size);

    /* 返回写入字节数 */
    return len;
}

/* sysfs属性：读取BAR空间寄存器 */
static ssize_t bar_read_store(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t count)
{
    /* 获取设备私有数据 */
    struct pcie_dev *edev = dev_get_drvdata(dev);
    /* 偏移和读取值 */
    u32 offset, val;
    /* 读取大小 */
    u8 size;
    int ret;

    /* 检查BAR是否已映射 */
    if (!edev || !edev->bar_mmap) {
        pr_err("BAR 未映射\n");
        return -ENODEV;
    }

    /* 获取锁 */
    ret = mutex_lock_interruptible(&edev->lock);
    if (ret)
        return ret;

    /* 解析用户输入 */
    if (sscanf(buf, "%x %hhu", &offset, &size) != 2) {
        pr_err("格式错误: 应为 'offset size'\n");
        mutex_unlock(&edev->lock);
        return -EINVAL;
    }

    /* 检查参数有效性 */
    if (offset >= edev->bar_size || (size != 1 && size != 2 && size != 4)) {
        pr_err("无效参数: offset=0x%04X size=%d\n", offset, size);
        mutex_unlock(&edev->lock);
        return -EINVAL;
    }

    /* 根据大小读取BAR空间 */
    switch (size) {
    case 1: {
        /* 读取1字节 */
        u8 val8 = readb(edev->bar_mmap + offset);
        /* 打印结果 */
        pr_info("BAR[0x%04X]: 0x%02X\n", offset, val8);
        break;
    }
    /* 读取2字节 */
    case 2: {
        /* 读取2字节 */
        u16 val16 = readw(edev->bar_mmap + offset);
        /* 打印结果 */
        pr_info("BAR[0x%04X]: 0x%04X\n", offset, val16);
        break;
    }
    /* 读取4字节 */
    case 4:
        /* 读取4字节 */
        val = readl(edev->bar_mmap + offset);
        /* 打印结果 */
        pr_info("BAR[0x%04X]: 0x%08X\n", offset, val);
        break;
    }

    /* 释放锁 */
    mutex_unlock(&edev->lock);

    /* 返回读取字节数 */
    return count;
}

/* sysfs属性：写入BAR空间寄存器 */
static ssize_t bar_write_store(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t count)
{
    /* 获取设备私有数据 */
    struct pcie_dev *edev = dev_get_drvdata(dev);
    /* 偏移和写入值 */
    u32 offset, value;
    /* 写入大小 */
    u8 size;
    int ret;

    /* 检查BAR是否已映射 */
    if (!edev || !edev->bar_mmap) {
        pr_err("BAR 未映射\n");
        return -ENODEV;
    }

    /* 获取锁 */
    ret = mutex_lock_interruptible(&edev->lock);
    if (ret)
        return ret;

    /* 解析用户输入 */
    if (sscanf(buf, "%x %hhu %x", &offset, &size, &value) != 3) {
        pr_err("格式错误: 应为 'offset size value'\n");
        mutex_unlock(&edev->lock);
        return -EINVAL;
    }

    /* 检查参数有效性 */
    if (offset >= edev->bar_size || (size != 1 && size != 2 && size != 4)) {
        pr_err("无效参数: offset=0x%04X size=%d\n", offset, size);
        mutex_unlock(&edev->lock);
        return -EINVAL;
    }

    /* 白名单检查，防止误操作 */
    if (!is_bar_writable(offset)) {
        pr_err("BAR寄存器 0x%04X 不在白名单中，禁止写入\n", offset);
        pr_info("允许写入的BAR寄存器:\n");
        pr_info("  0x00B4 - RTK_PCI_HISR0 中断状态0\n");
        pr_info("  0x00BC - RTK_PCI_HISR1 中断状态1\n");
        pr_info("  0x0300 - RTK_PCI_CTRL DMA控制\n");
        mutex_unlock(&edev->lock);
        return -EPERM;
    }

    /* 根据大小写入BAR空间 */
    switch (size) {
    /* 写入1字节 */
    case 1:
        writeb((u8)value, edev->bar_mmap + offset);
        break;
    /* 写入2字节 */
    case 2:
        writew((u16)value, edev->bar_mmap + offset);
        break;
    /* 写入4字节 */
    case 4:
        writel(value, edev->bar_mmap + offset);
        break;
    }

    /* 打印写入结果 */
    pr_info("BAR_WRITE[0x%04X]: 0x%08X (size=%d)\n", offset, value, size);

    /* 释放锁 */
    mutex_unlock(&edev->lock);

    /* 返回写入字节数 */
    return count;
}

/* sysfs属性 */
static DEVICE_ATTR(config_space, 0444, config_space_show, NULL);  /* 显示配置空间信息只读属性 */
static DEVICE_ATTR(config_read, 0200, NULL, config_read_store);   /* 配置空间寄存器读取只写属性 */
static DEVICE_ATTR(config_write, 0200, NULL, config_write_store); /* 配置空间寄存器写入只写属性 */
static DEVICE_ATTR(bar_info, 0444, bar_info_show, NULL);          /* 显示BAR空间信息只读属性 */
static DEVICE_ATTR(bar_read, 0200, NULL, bar_read_store);         /* BAR空间读取只写属性 */
static DEVICE_ATTR(bar_write, 0200, NULL, bar_write_store);       /* BAR空间写入只写属性 */

/* 将属性数组添加到组 */
static struct attribute *pcie_exp_attrs[] = {
    &dev_attr_config_space.attr,
    &dev_attr_config_read.attr,
    &dev_attr_config_write.attr,
    &dev_attr_bar_info.attr,
    &dev_attr_bar_read.attr,
    &dev_attr_bar_write.attr,
    NULL,
};

/* 属性组定义 */
static const struct attribute_group pcie_exp_group = {
    .attrs = pcie_exp_attrs,
};

/* 设备探测 */
static int pcie_explorer_probe(struct pci_dev *pdev,
                const struct pci_device_id *id)
{
    /* 设备私有数据指针 */
    struct pcie_dev *edev;
    /* 返回值 */
    int ret;

    /* 打印设备发现信息 */
    pr_info("%s: 发现设备 %s    \n", DRV_NAME, pci_name(pdev));

    /* 分配设备私有数据结构 */
    edev = kzalloc(sizeof(*edev), GFP_KERNEL);
    if (!edev)
        return -ENOMEM;

    /* 初始化互斥锁 */
    mutex_init(&edev->lock);

    /* 保存PCI设备指针 */
    edev->pdev = pdev;
    /* 将私有数据绑定到设备 */
    pci_set_drvdata(pdev, edev);

    /* 使能PCI设备内存资源 */
    ret = pci_enable_device_mem(pdev);
    if (ret) {
        pr_err("pci_enable_device_mem 失败: %d\n", ret);
        goto err_free;
    }

    pr_info("------------ 配置空间解析 ------------\n");
    /* 解析配置空间信息 */
    print_config_space(pdev);
    /* 解析标准Capability */
    parse_capabilities(pdev);
    /* 解析扩展Capability */
    parse_extended_capabilities(pdev);

    pr_info("------------ BAR空间解析 ------------\n");
    /* 解析BAR空间信息 */
    parse_bar_info(edev);
    /* 映射并探索BAR2空间 */
    map_and_explore_bar(edev);

    /* 创建sysfs属性组 */
    ret = devm_device_add_group(&pdev->dev, &pcie_exp_group);
    if (ret)
        goto err_disable;

    /* 打印初始化完成信息 */
    pr_info("%s: 设备 %s 初始化完成    \n", DRV_NAME, pci_name(pdev));

    return 0;

err_disable:
    pci_disable_device(pdev);  /* 禁用PCI设备 */
err_free:
    kfree(edev);  /* 释放设备私有数据 */
    return ret;
}

static void pcie_explorer_remove(struct pci_dev *pdev)
{
    /* 获取设备私有数据 */
    struct pcie_dev *edev = pci_get_drvdata(pdev);

    /* 打印设备移除信息 */
    pr_info("%s: 移除设备 %s    \n", DRV_NAME, pci_name(pdev));

    /* 检查BAR是否已映射 */
    if (edev && edev->bar_mmap)
        pci_iounmap(pdev, edev->bar_mmap);  /* 解除BAR映射 */

    /* 移除sysfs属性组 */
    device_remove_group(&pdev->dev, &pcie_exp_group);

    /* 禁用PCI设备 */
    pci_disable_device(pdev);

    /* 销毁互斥锁 */
    mutex_destroy(&edev->lock);

    /* 释放设备私有数据 */
    kfree(edev);

    /* 打印移除完成信息 */
    pr_info("%s: 设备 %s 已移除    \n", DRV_NAME, pci_name(pdev));
}

static int pcie_explorer_suspend(struct pci_dev *pdev, pm_message_t state)
{
    /* 打印挂起信息 */
    pr_info("%s: suspend %s\n", DRV_NAME, pci_name(pdev));

    /* 保存PCI设备状态 */
    pci_save_state(pdev);
    /* 禁用PCI设备 */
    pci_disable_device(pdev);

    return 0;
}

static int pcie_explorer_resume(struct pci_dev *pdev)
{
    int ret;

    /* 打印恢复信息 */
    pr_info("%s: resume %s\n", DRV_NAME, pci_name(pdev));

    /* 恢复PCI设备状态 */
    pci_restore_state(pdev);
    /* 重新使能设备内存资源 */
    ret = pci_enable_device_mem(pdev);
    if (ret)
        pr_err("pci_enable_device_mem 失败: %d\n", ret);

    return ret;
}

/* PCI设备ID表 */
static const struct pci_device_id pcie_explorer_ids[] = {
    { PCI_DEVICE(0x10EC, 0x8179) },  /* RTL8188EE */
    { PCI_DEVICE(0x10EC, 0xC821) },  /* RTL8821CE */
    { PCI_DEVICE(0x10EC, 0xC822) },  /* RTL8822CE */
    { PCI_DEVICE(0x10EC, 0xC82F) },  /* RTL8822CE */
    { PCI_DEVICE(0x10EC, 0xb852) },  /* RTL8852BE */
    { PCI_DEVICE(0x10EC, 0xb85b) },  /* RTL8852BE */
    { }
};
/* 导出设备ID表到用户空间 */
MODULE_DEVICE_TABLE(pci, pcie_explorer_ids);

static struct pci_driver pcie_explorer_driver = {
    .name     = DRV_NAME,
    .id_table = pcie_explorer_ids,
    .probe    = pcie_explorer_probe,
    .remove   = pcie_explorer_remove,
    .suspend  = pcie_explorer_suspend,
    .resume   = pcie_explorer_resume,
};
/* 自动注册/注销PCI驱动 */
module_pci_driver(pcie_explorer_driver);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("pcie_explorer module");
MODULE_LICENSE("GPL v2");