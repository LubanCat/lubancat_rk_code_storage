/*
 * PCIe DMA机制实验驱动
 *
 * 参考rtw88/pci.c的DMA实现：
 *   - rtw_pci_init_rx_ring()    -> RX描述符环初始化
 *   - rtw_pci_reset_rx_desc()   -> RX描述符重置
 *   - rtw_pci_reset_buf_desc()  -> 环基地址/大小写入硬件寄存器
 *   - rtw_pci_dma_reset()       -> DMA引擎复位
 *   - rtw_pci_free_rx_ring()    -> RX环释放
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/skbuff.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/pci_regs.h>

#define DRV_NAME    "pcie_dma"  /* 驱动名称 */
#define DRV_VERSION "1.0"       /* 驱动版本号 */

/* 以下寄存器地址全部来自rtw88驱动的pci.h */

/* DMA控制寄存器 */
#define RTK_PCI_CTRL              0x300     /* DMA控制寄存器基地址 */
#define BIT_RST_TRXDMA_INTF       BIT(20)   /* 复位TX/RX DMA接口 */
#define BIT_RX_TAG_EN             BIT(15)   /* 使能RX Tag */

/* RX DMA描述符环寄存器 */
#define RTK_PCI_RXBD_DESA_MPDUQ   0x338     /* RX环基地址 */
#define RTK_PCI_RXBD_NUM_MPDUQ    0x382     /* RX环描述符数量 */
#define RTK_PCI_RXBD_IDX_MPDUQ    0x3B4     /* RX环索引 */

/* DMA环大小寄存器 */
#define RTK_PCI_TXBD_NUM_VOQ      0x384     /* VO队列环大小 */
#define RTK_PCI_TXBD_NUM_VIQ      0x386     /* VI队列环大小 */
#define RTK_PCI_TXBD_NUM_BEQ      0x388     /* BE队列环大小 */
#define RTK_PCI_TXBD_NUM_BKQ      0x38A     /* BK队列环大小 */
#define RTK_PCI_TXBD_NUM_MGMTQ    0x380     /* 管理队列环大小 */
#define RTK_PCI_TXBD_NUM_HI0Q     0x38C     /* 高优先级队列环大小 */
#define RTK_PCI_TXBD_NUM_H2CQ     0x1328    /* H2C队列环大小 */

/* DMA索引寄存器*/
#define RTK_PCI_TXBD_IDX_VOQ      0x3A0     /* VO队列索引寄存器 */
#define RTK_PCI_TXBD_IDX_VIQ      0x3A4     /* VI队列索引寄存器 */
#define RTK_PCI_TXBD_IDX_BEQ      0x3A8     /* BE队列索引寄存器 */
#define RTK_PCI_TXBD_IDX_BKQ      0x3AC     /* BK队列索引寄存器 */
#define RTK_PCI_TXBD_IDX_MGMTQ    0x3B0     /* 管理队列索引寄存器 */
#define RTK_PCI_TXBD_IDX_HI0Q     0x3B8     /* 高优先级队列索引寄存器 */
#define RTK_PCI_TXBD_IDX_H2CQ     0x132C    /* H2C队列索引寄存器 */

/* 读写指针清零寄存器 */
#define RTK_PCI_TXBD_RWPTR_CLR    0x39C     /* 写0xFFFFFFFF清零所有环的读写指针 */

/* 描述符索引掩码 */
#define TRX_BD_IDX_MASK           GENMASK(11, 0)   /* 索引低12位有效 */
#define TRX_BD_HW_IDX_MASK        GENMASK(27, 16)  /* 硬件索引在bit[27:16] */

/* RX 缓冲区大小和描述符环大小 */
#define RTK_PCI_RX_BUF_SIZE      (11454 + 24)  /* 每个RX数据缓冲区大小11478字节 */
#define RTK_MAX_RX_DESC_NUM       512          /* RX描述符环最大描述符数量 */

/* RX DMA描述符结构体 */
struct rtw_pci_rx_buffer_desc {
    __le16 buf_size;        /* 缓冲区容量 */
    __le16 total_pkt_size;  /* 接收到的总包大小 */
    __le32 dma;             /* 数据缓冲区DMA物理地址 */
};

/* 设备私有数据结构体  */
struct dma_exp_dev {
    struct pci_dev *pdev;       /* PCI设备指针，用于访问配置空间 */
    void __iomem *bar_mmap;     /* BAR2 MMIO映射后的虚拟地址 */
    size_t bar_size;            /* BAR2 空间大小 */
    struct mutex lock;          /* 互斥锁 */

    /* RX描述符环，一致性DMA内存 */
    void *rx_ring_head;        /* 描述符环的内核虚拟地址 */
    dma_addr_t rx_ring_dma;    /* 描述符环的DMA物理地址 */
    size_t rx_ring_size;       /* 描述符环总大小 */
    u32 rx_ring_len;           /* 描述符环中的描述符数量 */

    /* RX数据缓冲区，每个描述符对应一个skb */
    struct sk_buff *rx_buf[RTK_MAX_RX_DESC_NUM]; /* 保存每个描述符对应的skb指针 */

    /* DMA状态 */
    bool dma_initialized; /* DMA环是否已成功初始化 */
    u32 rx_wp;            /* 写指针，表示下一个可用描述符 */
    u32 rx_rp;            /* 读指针，表示硬件已处理到的位置 */
};

/*
 * 重置单个RX描述符
 * 参考pci.c：rtw_pci_reset_rx_desc()
 */
static int dma_exp_reset_rx_desc(struct dma_exp_dev *edev,
                                 struct sk_buff *skb,
                                 u32 idx)
{
    /* 获取PCI设备指针 */
    struct pci_dev *pdev = edev->pdev;
    /* 描述符指针 */
    struct rtw_pci_rx_buffer_desc *buf_desc;
    /* 缓冲区大小 */
    int buf_sz = RTK_PCI_RX_BUF_SIZE;
    /* DMA物理地址 */
    dma_addr_t dma;

    /* 检查skb是否有效 */
    if (!skb)
        return -EINVAL;

    /*
     * 将skb数据区映射为DMA地址
     * 参考pci.c：rtw_pci_reset_rx_desc()
     *   dma = dma_map_single(&pdev->dev, skb->data, buf_sz, DMA_FROM_DEVICE);
     *
     * DMA_FROM_DEVICE：表示数据流方向是设备写入内存（RX方向）
     */
    dma = dma_map_single(&pdev->dev, skb->data, buf_sz,
                         DMA_FROM_DEVICE);
    if (dma_mapping_error(&pdev->dev, dma))  /* 检查映射是否成功 */
        return -EBUSY;

    /*
     * 将DMA地址暂存在skb->cb控制块中
     * 参考pci.c：rtw_pci_reset_rx_desc()
     *   *((dma_addr_t *)skb->cb) = dma;
     *
     * 后续释放时需要用此地址调用dma_unmap_single
     */
    *((dma_addr_t *)skb->cb) = dma;

    /*
     * 定位到描述符环中第idx个描述符并填写
     * 参考pci.c：rtw_pci_reset_rx_desc()
     *  buf_desc = (struct rtw_pci_rx_buffer_desc *)(rx_ring->r.head +
     *                          idx * desc_sz);
     *  memset(buf_desc, 0, sizeof(*buf_desc));
     *  buf_desc->buf_size = cpu_to_le16(RTK_PCI_RX_BUF_SIZE);
     *  buf_desc->dma = cpu_to_le32(dma);
     */
    buf_desc = (struct rtw_pci_rx_buffer_desc *)
               (edev->rx_ring_head +                          /* 环起始虚拟地址 */
                idx * sizeof(struct rtw_pci_rx_buffer_desc)); /* 偏移到第idx个 */
    memset(buf_desc, 0, sizeof(*buf_desc));                   /* 清零描述符 */
    buf_desc->buf_size = cpu_to_le16(buf_sz);                 /* 写入缓冲区容量，小端 */
    buf_desc->dma = cpu_to_le32(dma);                         /* 写入DMA物理地址，小端 */

    return 0;
}

/*
 * RX环完整初始化
 * 参考 pci.c：rtw_pci_init_rx_ring()
 */
static int dma_exp_rx_ring_init(struct dma_exp_dev *edev)
{
    /* PCI设备指针 */
    struct pci_dev *pdev = edev->pdev;
    /* 临时skb指针 */
    struct sk_buff *skb;
    /* 描述符环总大小 */
    int ring_sz;
    /* 每个缓冲区大小 */
    int buf_sz = RTK_PCI_RX_BUF_SIZE;
    /* 描述符数量 */
    u32 len = RTK_MAX_RX_DESC_NUM;
    /* 循环变量和已分配计数 */
    int i, allocated;
    /* 返回值 */
    int ret = 0;

    pr_info("  [RX环] 描述符数量: %u\n", len);             /* 打印描述符数量 */
    pr_info("  [RX环] 描述符大小: %zu bytes\n",            /* 打印单个描述符大小 */
            sizeof(struct rtw_pci_rx_buffer_desc));
    pr_info("  [RX环] 数据缓冲区: %d bytes/个\n", buf_sz); /* 打印每个缓冲区大小 */

    /*
     * 步骤1：分配描述符环（一致性DMA内存）
     *
     * 参考pci.c：rtw_pci_init_rx_ring()
     *   head = dma_alloc_coherent(&pdev->dev, ring_sz, &dma, GFP_KERNEL);
     */
    ring_sz = sizeof(struct rtw_pci_rx_buffer_desc) * len;       /* 计算环总大小 */
    edev->rx_ring_head = dma_alloc_coherent(&pdev->dev, ring_sz, /* 分配一致性DMA内存 */
                                            &edev->rx_ring_dma,
                                            GFP_KERNEL);
    if (!edev->rx_ring_head) {
        pr_err("  [RX环] 描述符环分配失败\n");
        return -ENOMEM;
    }
    /* 保存环总大小 */
    edev->rx_ring_size = ring_sz;
    /* 保存描述符数量 */
    edev->rx_ring_len = len;

    pr_info("  [RX环] 虚拟地址: %p\n", edev->rx_ring_head);   /* 打印虚拟地址 */
    pr_info("  [RX环] DMA地址:  %pad\n", &edev->rx_ring_dma); /* 打印DMA物理地址 */
    pr_info("  [RX环] 总大小:   %d bytes\n", ring_sz);        /* 打印环总大小 */

    /*
     * 步骤2：为每个描述符分配数据缓冲区并映射DMA
     *
     * 参考 pci.c：rtw_pci_init_rx_ring() 中的循环
     *   for (i = 0; i < len; i++) {
     *       skb = dev_alloc_skb(buf_sz);
     *       if (!skb) { allocated = i; ret = -ENOMEM; goto err_out; }
     *       memset(skb->data, 0, buf_sz);
     *       rx_ring->buf[i] = skb;
     *       ret = rtw_pci_reset_rx_desc(rtwdev, skb, rx_ring, i, desc_size);
     *       if (ret) { allocated = i; dev_kfree_skb_any(skb); goto err_out; }
     *   }
     */
    for (i = 0; i < (int)len; i++) {                       /* 遍历每个描述符 */
        skb = dev_alloc_skb(buf_sz);                       /* 分配一个网络缓冲区 */
        if (!skb) {                                        /* 判断是否分配失败 */
            allocated = i;                                 /* 记录已分配到第几个 */
            ret = -ENOMEM;                                 /* 设置错误码 */
            goto err_out;                                  /* 跳转到错误处理 */
        }
        memset(skb->data, 0, buf_sz);                      /* 清零缓冲区数据 */
        edev->rx_buf[i] = skb;                             /* 保存skb指针到数组 */

        ret = dma_exp_reset_rx_desc(edev, skb, i);         /* 映射DMA并填写描述符 */
        if (ret) {                                         /* 判断是否映射失败 */
            allocated = i;                                 /* 记录已分配到第几个 */
            dev_kfree_skb(skb);                            /* 释放当前skb */
            edev->rx_buf[i] = NULL;                        /* 清空指针 */
            goto err_out;                                  /* 跳转到错误处理 */
        }

        /* 只打印前4个描述符的详细信息，避免日志过多 */
        if (i < 4) {                                       /* 前4个描述符 */
            struct rtw_pci_rx_buffer_desc *desc =          /* 获取描述符指针 */
                (struct rtw_pci_rx_buffer_desc *)
                (edev->rx_ring_head +
                 i * sizeof(struct rtw_pci_rx_buffer_desc));
            pr_info("  [RX环] 前4个描述符的详细信息: \n");
            pr_info("  [RX环] 描述符[%d]: dma=0x%08X buf_size=%d\n",
                    i,                                     /* 描述符索引 */
                    le32_to_cpu(desc->dma),                /* DMA地址 */
                    le16_to_cpu(desc->buf_size));          /* 缓冲区大小 */
        }
    }
    pr_info("  [RX环] 共%u个描述符已初始化\n", len);       /* 打印初始化完成信息 */

    /* 初始化写指针为0 */
    edev->rx_wp = 0;
    /* 初始化读指针为0 */
    edev->rx_rp = 0;

    return 0;

err_out:
    /*
     * 回退：释放已分配的资源
     *
     * 参考pci.c：rtw_pci_init_rx_ring() 的err_out标签
     *   for (i = 0; i < allocated; i++) {
     *       skb = rx_ring->buf[i];
     *       if (!skb) continue;
     *       dma = *((dma_addr_t *)skb->cb);
     *       dma_unmap_single(&pdev->dev, dma, buf_sz, DMA_FROM_DEVICE);
     *       dev_kfree_skb_any(skb);
     *       rx_ring->buf[i] = NULL;
     *   }
     *   dma_free_coherent(&pdev->dev, ring_sz, head, dma);
     */
    for (i = 0; i < allocated; i++) {                      /* 遍历已分配的描述符 */
        dma_addr_t dma;                                    /* 临时DMA地址 */
        skb = edev->rx_buf[i];                             /* 获取skb指针 */
        if (!skb)                                          /* 跳过空的 */
            continue;
        dma = *((dma_addr_t *)skb->cb);                    /* 从cb中取出DMA地址 */
        dma_unmap_single(&pdev->dev, dma, buf_sz,          /* 解除DMA映射 */
                         DMA_FROM_DEVICE);
        dev_kfree_skb(skb);                                /* 释放skb */
        edev->rx_buf[i] = NULL;                            /* 清空指针 */
    }
    dma_free_coherent(&pdev->dev, ring_sz,                 /* 释放描述符环一致性内存 */
                      edev->rx_ring_head, edev->rx_ring_dma);
    edev->rx_ring_head = NULL;                             /* 清空环指针 */
    pr_err("  [RX环] 初始化失败\n");                       /* 打印错误信息 */
    return ret;                                            /* 返回错误码 */
}

/*
 * 配置硬件DMA寄存器
 * 参考pci.c：rtw_pci_reset_buf_desc()、rtw_pci_dma_reset()
 */
static void dma_exp_configure_hw(struct dma_exp_dev *edev)
{
    pr_info("  [HW配置] 写入DMA寄存器:\n");              /* 打印标题 */

    /*
     * 写RX环基地址，告诉硬件描述符环在哪个物理地址
     *
     * 参考pci.c：rtw_pci_reset_buf_desc()
     *   dma = rtwpci->rx_rings[RTW_RX_QUEUE_MPDU].r.dma;
     *   rtw_write32(rtwdev, RTK_PCI_RXBD_DESA_MPDUQ, dma);
     *
     * 硬件通过此地址找到描述符环，再逐个读取描述符获取数据缓冲区地址
     */
    writel((u32)edev->rx_ring_dma,                         /* 写入DMA物理地址 */
           edev->bar_mmap + RTK_PCI_RXBD_DESA_MPDUQ);      /* 寄存器偏移0x338 */
    pr_info("    REG[0x%04X] RX环基地址 = 0x%08X\n",       /* 打印写入结果 */
            RTK_PCI_RXBD_DESA_MPDUQ,                       /* 寄存器地址 */
            (u32)edev->rx_ring_dma);                       /* 写入的值 */

    /*
     * 写RX环大小，告诉硬件环中有多少个描述符
     *
     * 参考pci.c：rtw_pci_reset_buf_desc()
     *   len = rtwpci->rx_rings[RTW_RX_QUEUE_MPDU].r.len;
     *   rtw_write16(rtwdev, RTK_PCI_RXBD_NUM_MPDUQ,
     *               len & TRX_BD_IDX_MASK);
     *
     * 硬件根据此值判断环的边界，防止越界访问
     */
    writew((u16)(edev->rx_ring_len & TRX_BD_IDX_MASK),     /* 写入描述符数量 */
           edev->bar_mmap + RTK_PCI_RXBD_NUM_MPDUQ);       /* 寄存器偏移0x382 */
    pr_info("    REG[0x%04X] RX环大小   = %u\n",           /* 打印写入结果 */
            RTK_PCI_RXBD_NUM_MPDUQ,                        /* 寄存器地址 */
            edev->rx_ring_len);                            /* 写入的值 */

    /*
     * 清零读写指针，复位所有DMA环的索引
     *
     * 参考 pci.c：rtw_pci_reset_buf_desc()
     *   rtw_write32(rtwdev, RTK_PCI_TXBD_RWPTR_CLR, 0xffffffff);
     *
     * 写0xFFFFFFFF会将所有TX/RX环的读写指针归零
     */
    writel(0xffffffff,                                      /* 写入全1清零所有指针 */
           edev->bar_mmap + RTK_PCI_TXBD_RWPTR_CLR);        /* 寄存器偏移0x39C */
    pr_info("    REG[0x%04X] 读写指针=0xFFFFFFFF (清零)\n", /* 打印写入结果 */
            RTK_PCI_TXBD_RWPTR_CLR);                        /* 寄存器地址 */

    /*
     * 复位DMA引擎并使能RX Tag
     *
     * 参考pci.c：rtw_pci_dma_reset()
     *   static void rtw_pci_dma_reset(struct rtw_dev *rtwdev,
     *                                 struct rtw_pci *rtwpci)
     *   {
     *       rtw_write32_set(rtwdev, RTK_PCI_CTRL,
     *                       BIT_RST_TRXDMA_INTF | BIT_RX_TAG_EN);
     *       rtwpci->rx_tag = 0;
     *   }
     *
     * BIT_RST_TRXDMA_INTF：复位DMA引擎状态机
     * BIT_RX_TAG_EN：使能RX Tag校验，用于检测DMA超时
     */
    {
        u32 ctrl = readl(edev->bar_mmap + RTK_PCI_CTRL);   /* 先读取当前控制寄存器值 */
        ctrl |= BIT_RST_TRXDMA_INTF | BIT_RX_TAG_EN;       /* 置位复位和RX Tag位 */
        writel(ctrl, edev->bar_mmap + RTK_PCI_CTRL);       /* 写回控制寄存器 */
        pr_info("    REG[0x%04X] DMA控制    = 0x%08X (复位+RX Tag)\n", /* 打印结果 */
                RTK_PCI_CTRL,                              /* 寄存器地址 */
                ctrl);                                     /* 写入的值 */
    }

    pr_info("  [HW配置] 完成\n");                          /* 打印完成信息 */

    /*
     * 设备未加载firmware，DMA不会真正传输数据，并且前后还有大量必需步骤才能真正实现
     * 但描述符环和寄存器配置是完整的
     */
}

/* 
 * DMA子系统清理
 *
 * 参考 pci.c：rtw_pci_free_rx_ring_skbs()、rtw_pci_free_rx_ring()
 */
static void dma_exp_cleanup(struct dma_exp_dev *edev)
{
    /* PCI设备指针 */
    struct pci_dev *pdev = edev->pdev;
    /* 缓冲区大小 */
    int buf_sz = RTK_PCI_RX_BUF_SIZE;
    /* 循环变量 */
    int i;

    /* 未初始化则直接返回 */
    if (!edev->dma_initialized)
        return;

    /*
     * 释放每个描述符的数据缓冲区
     *
     * 参考pci.c：rtw_pci_free_rx_ring_skbs()
     *   for (i = 0; i < rx_ring->r.len; i++) {
     *       skb = rx_ring->buf[i];
     *       if (!skb) continue;
     *       dma = *((dma_addr_t *)skb->cb);
     *       dma_unmap_single(&pdev->dev, dma, buf_sz, DMA_FROM_DEVICE);
     *       dev_kfree_skb(skb);
     *       rx_ring->buf[i] = NULL;
     *   }
     */
    for (i = 0; i < (int)edev->rx_ring_len; i++) {         /* 遍历每个描述符 */
        dma_addr_t dma;                                    /* 临时DMA地址 */
        if (!edev->rx_buf[i])                              /* 跳过空的缓冲区 */
            continue;
        dma = *((dma_addr_t *)edev->rx_buf[i]->cb);        /* 从cb取出DMA地址 */
        dma_unmap_single(&pdev->dev, dma, buf_sz,          /* 解除DMA映射 */
                         DMA_FROM_DEVICE);
        dev_kfree_skb(edev->rx_buf[i]);                    /* 释放skb */
        edev->rx_buf[i] = NULL;                            /* 清空指针 */
    }

    /*
     * 释放描述符环
     *
     * 参考pci.c：rtw_pci_free_rx_ring()
     *   dma_free_coherent(&pdev->dev, ring_sz, head, rx_ring->r.dma);
     */
    if (edev->rx_ring_head) {                              /* 检查环是否已分配 */
        dma_free_coherent(&pdev->dev,                      /* 释放一致性DMA内存 */
                          edev->rx_ring_size,              /* 环大小 */
                          edev->rx_ring_head,              /* 虚拟地址 */
                          edev->rx_ring_dma);              /* DMA物理地址 */
        edev->rx_ring_head = NULL;                         /* 清空指针 */
    }

    edev->dma_initialized = false;                         /* 标记为未初始化 */
    pr_info("DMA资源已释放\n");                            /* 打印完成信息 */
}

/* sysfs属性接口: 显示DMA总体信息 */
static ssize_t dma_info_show(struct device *dev,
                             struct device_attribute *attr,
                             char *buf)
{
    /* 获取设备私有数据 */
    struct dma_exp_dev *edev = dev_get_drvdata(dev);
    /* 已写入长度 */
    int len = 0;

    /* 检查私有数据 */
    if (!edev)
        return -ENODEV;

    len += sprintf(buf + len, "------PCI DMA信息------\n");  /* 标题 */
    len += sprintf(buf + len, "DMA是否始化: %s\n",         /* 初始化状态 */
                   edev->dma_initialized ? "Yes" : "No");

    if (!edev->dma_initialized)                            /* 未初始化则提前返回 */
        return len;

    len += sprintf(buf + len, "\nRX 描述符环:\n");         /* 描述符环信息标题 */
    len += sprintf(buf + len, "  虚拟地址:   %p\n",        /* 虚拟地址 */
                   edev->rx_ring_head);
    len += sprintf(buf + len, "  DMA地址:    %pad\n",      /* DMA物理地址 */
                   &edev->rx_ring_dma);
    len += sprintf(buf + len, "  环大小:     %zu bytes\n", /* 环总大小 */
                   edev->rx_ring_size);
    len += sprintf(buf + len, "  描述符数:   %u\n",        /* 描述符数量 */
                   edev->rx_ring_len);
    len += sprintf(buf + len, "  描述符大小: %zu bytes\n", /* 单个描述符大小 */
                   sizeof(struct rtw_pci_rx_buffer_desc));
    len += sprintf(buf + len, "  WP: %u  RP: %u\n",        /* 读写指针 */
                   edev->rx_wp, edev->rx_rp);

    /* 读取硬件RX索引寄存器 */
    if (edev->bar_mmap) {                                  /* BAR已映射 */
        u32 rx_idx = readl(edev->bar_mmap +                /* 读取RX索引寄存器 */
                           RTK_PCI_RXBD_IDX_MPDUQ);
        len += sprintf(buf + len, "\n硬件寄存器:\n");      /* 硬件寄存器标题 */
        len += sprintf(buf + len, "  RX_IDX [0x%04X] = 0x%08X\n", /* 索引寄存器值 */
                       RTK_PCI_RXBD_IDX_MPDUQ, rx_idx);
        len += sprintf(buf + len, "    HW WP: %u\n",       /* 硬件写指针 */
                       (u32)((rx_idx >> 16) & TRX_BD_IDX_MASK)); /* bit[27:16] */
        len += sprintf(buf + len, "    Host RP: %u\n",     /* 主机读指针 */
                       (u32)(rx_idx & TRX_BD_IDX_MASK));   /* bit[11:0] */
        len += sprintf(buf + len, "  RX_DESA [0x%04X] = 0x%08X\n", /* 环基地址读回 */
                       RTK_PCI_RXBD_DESA_MPDUQ,
                       readl(edev->bar_mmap + RTK_PCI_RXBD_DESA_MPDUQ));
        len += sprintf(buf + len, "  RX_NUM  [0x%04X] = 0x%04X\n", /* 环大小读回 */
                       RTK_PCI_RXBD_NUM_MPDUQ,
                       readw(edev->bar_mmap + RTK_PCI_RXBD_NUM_MPDUQ));
    }

    /* 返回写入的总字节数 */
    return len;
}

/* sysfs属性接口: 显示RX描述符环内容 */
static ssize_t dma_ring_show(struct device *dev,
                             struct device_attribute *attr,
                             char *buf)
{
    /* 获取设备私有数据 */
    struct dma_exp_dev *edev = dev_get_drvdata(dev);
    /* 已写入长度 */
    int len = 0;
    /* 循环变量和显示数量 */
    int i, show_count;

    /* 未初始化检查 */
    if (!edev || !edev->dma_initialized)
        return sprintf(buf, "DMA未初始化\n");

    /* 只显示前16个 */
    show_count = min_t(int, edev->rx_ring_len, 16);

    len += sprintf(buf + len,                              /* 打印标题 */
                   "RX 描述符环 (显示前 %d / %u 个):\n",
                   show_count, edev->rx_ring_len);
    len += sprintf(buf + len,                              /* 打印表头 */
                   "%-6s %-12s %-12s %-12s\n",
                   "IDX", "buf_size", "total_pkt", "dma_addr");
    len += sprintf(buf + len,                              /* 打印分隔线 */
                   "----------------------------------------------\n");

    /* 遍历要显示的描述符 */
    for (i = 0; i < show_count; i++) {
        /* 获取描述符指针 */
        struct rtw_pci_rx_buffer_desc *desc =
            (struct rtw_pci_rx_buffer_desc *)
            (edev->rx_ring_head +
             i * sizeof(struct rtw_pci_rx_buffer_desc));

        len += sprintf(buf + len, "%-6d %-12u %-12u 0x%08X\n", /* 格式化输出 */
                       i,                                  /* 描述符索引 */
                       le16_to_cpu(desc->buf_size),        /* 缓冲区容量 */
                       le16_to_cpu(desc->total_pkt_size),  /* 接收包大小 */
                       le32_to_cpu(desc->dma));            /* DMA地址 */
    }

    /* 返回写入的总字节数 */
    return len;
}

/* sysfs属性定义 */
static DEVICE_ATTR(dma_info, 0444, dma_info_show, NULL);   /* DMA信息，只读 */
static DEVICE_ATTR(dma_ring, 0444, dma_ring_show, NULL);   /* 描述符环内容，只读 */

/* 属性数组 */
static struct attribute *dma_exp_attrs[] = {
    &dev_attr_dma_info.attr,                               /* dma_info属性 */
    &dev_attr_dma_ring.attr,                               /* dma_ring属性 */
    NULL,                                                  /* 数组结束标记 */
};

/* 属性组定义 */
static const struct attribute_group dma_exp_group = {
    .attrs = dma_exp_attrs,                                /* 指向属性数组 */
};

/*
 * 设备探测函数
 *
 * 参考pci.c：rtw_pci_probe()
 */
static int dma_exp_probe(struct pci_dev *pdev,
                         const struct pci_device_id *id)
{
    /* 设备私有数据 */
    struct dma_exp_dev *edev;
    /* 返回值 */
    int ret;

    /* 打印发现信息 */
    pr_info("%s: 发现设备 %s\n", DRV_NAME, pci_name(pdev));

    /* 分配并清零私有数据 */
    edev = kzalloc(sizeof(*edev), GFP_KERNEL);
    if (!edev)
        return -ENOMEM;

    /* 初始化互斥锁 */
    mutex_init(&edev->lock);
    /* 保存PCI设备指针 */
    edev->pdev = pdev;
    /* 绑定私有数据到PCI设备 */
    pci_set_drvdata(pdev, edev);

    /* 使能PCI设备 */
    ret = pci_enable_device(pdev);
    if (ret) {
        pr_err("pci_enable_device 失败: %d\n", ret);
        goto err_free;
    }
    /* 设置总线主控 */
    pci_set_master(pdev);

    /* 申请PCI区域 */
    ret = pci_request_regions(pdev, DRV_NAME);
    if (ret) {
        pr_err("pci_request_regions 失败: %d\n", ret);
        goto err_disable;
    }

    /*
     * 映射BAR2
     *
     * 参考 pci.c：rtw_pci_io_mapping()
     *   len = pci_resource_len(pdev, bar_id);
     *   rtwpci->mmap = pci_iomap(pdev, bar_id, len);
     *   if (!rtwpci->mmap) { pci_release_regions(pdev); return -ENOMEM; }
     */
    /* 获取BAR2大小 */
    edev->bar_size = pci_resource_len(pdev, 2);
    if (edev->bar_size == 0) {
        pr_err("BAR2大小为0\n");
        ret = -ENODEV;
        goto err_release;
    }
    /* 映射BAR2到虚拟地址 */
    edev->bar_mmap = pci_iomap(pdev, 2, edev->bar_size);
    if (!edev->bar_mmap) {
        pr_err("BAR2映射失败\n");
        ret = -ENOMEM;
        goto err_release;
    }
    /* 打印映射信息 */
    pr_info("  BAR2 映射: 物理=0x%llx 虚拟=%p 大小=%zu\n",
            (unsigned long long)pci_resource_start(pdev, 2), /* 物理地址 */
            edev->bar_mmap,                                  /* 虚拟地址 */
            edev->bar_size);                                 /* 大小 */

    pr_info("----------DMA初始化----------\n");

    /* 初始化RX DMA环 */
    ret = dma_exp_rx_ring_init(edev);
    if (ret) {
        pr_err("RX 环初始化失败: %d\n", ret);
        goto err_unmap;
    }

    /* 配置硬件DMA寄存器 */
    dma_exp_configure_hw(edev);

    /* 标记DMA已初始化 */
    edev->dma_initialized = true;

    /* 创建sysfs属性组 */
    ret = devm_device_add_group(&pdev->dev, &dma_exp_group);
    if (ret)
        goto err_dma_cleanup;

    pr_info("%s: 设备 %s 初始化完成\n",
            DRV_NAME, pci_name(pdev));
    return 0;

err_dma_cleanup:
    /* 释放DMA资源 */
    dma_exp_cleanup(edev);
err_unmap:
    /* 解除BAR映射 */
    pci_iounmap(pdev, edev->bar_mmap);
err_release:
    /* 释放PCI区域 */
    pci_release_regions(pdev);
err_disable:
    /* 禁用PCI设备 */
    pci_disable_device(pdev);
err_free:
    /* 释放私有数据 */
    kfree(edev);
    /* 返回错误码 */
    return ret;
}

/*
 * 设备移除函数
 *
 * 参考pci.c：rtw_pci_remove()
 */
static void dma_exp_remove(struct pci_dev *pdev)
{
    /* 获取私有数据 */
    struct dma_exp_dev *edev = pci_get_drvdata(pdev);

    /* 空指针检查 */
    if (!edev)
        return;

    /* 移除sysfs属性组 */
    device_remove_group(&pdev->dev, &dma_exp_group);
    /* 释放DMA资源 */
    dma_exp_cleanup(edev);

    /* 判断BAR是否映射 */
    if (edev->bar_mmap) {
        /* 解除BAR映射 */
        pci_iounmap(pdev, edev->bar_mmap);
        /* 清空指针 */
        edev->bar_mmap = NULL;
    }

    /* 释放PCI区域 */
    pci_release_regions(pdev);
    /* 禁用PCI设备 */
    pci_disable_device(pdev);
    /* 销毁互斥锁 */
    mutex_destroy(&edev->lock);
    /* 释放私有数据 */
    kfree(edev);

    pr_info("%s: 设备 %s 已移除\n", DRV_NAME, pci_name(pdev));
}

/* 系统挂起回调 */
static int dma_exp_suspend(struct device *dev)
{
    /* 转换为PCI设备 */
    struct pci_dev *pdev = to_pci_dev(dev);

    /* 打印挂起信息 */
    pr_info("%s: suspend\n", DRV_NAME);
    /* 保存PCI配置空间状态 */
    pci_save_state(pdev);
    /* 禁用PCI设备 */
    pci_disable_device(pdev);
    return 0;
}

/* 系统恢复回调 */
static int dma_exp_resume(struct device *dev)
{
    /* 转换为PCI设备 */
    struct pci_dev *pdev = to_pci_dev(dev);
    /* 返回值 */
    int ret;

    /* 打印恢复信息 */
    pr_info("%s: resume\n", DRV_NAME);
    /* 恢复PCI配置空间状态 */
    pci_restore_state(pdev);
    /* 重新使能PCI设备 */
    ret = pci_enable_device(pdev);
    if (ret)
        /* 使能失败，打印错误 */
        pr_err("resume: pci_enable_device 失败: %d\n", ret);
    else
        /* 使能成功，重新设置总线主控 */
        pci_set_master(pdev);
    return ret;
}

/* 使用dev_pm_ops接口定义电源管理回调 */
static SIMPLE_DEV_PM_OPS(dma_exp_pm_ops,    /* PM操作结构体名 */
                         dma_exp_suspend,   /* 挂起回调 */
                         dma_exp_resume);   /* 恢复回调 */

/* PCI设备ID匹配表 */
static const struct pci_device_id dma_exp_ids[] = {
    { PCI_DEVICE(0x10EC, 0x8179) },  /* RTL8188EE */
    { PCI_DEVICE(0x10EC, 0xC821) },  /* RTL8821CE */
    { PCI_DEVICE(0x10EC, 0xC822) },  /* RTL8822CE */
    { PCI_DEVICE(0x10EC, 0xC82F) },  /* RTL8822CE */
    { PCI_DEVICE(0x10EC, 0xb852) },  /* RTL8852BE */
    { PCI_DEVICE(0x10EC, 0xb85b) },  /* RTL8852BE */
    { }
};
/* 导出设备ID表到用户空间 */
MODULE_DEVICE_TABLE(pci, dma_exp_ids);

/* PCI驱动结构体 */
static struct pci_driver dma_exp_driver = {
    .name      = DRV_NAME,             /* 驱动名称 */
    .id_table  = dma_exp_ids,          /* 设备匹配表 */
    .probe     = dma_exp_probe,        /* 设备匹配成功后的初始化入口 */
    .remove    = dma_exp_remove,       /* 设备移除时的清理入口 */
    .driver.pm = &dma_exp_pm_ops,      /* 电源管理操作 */
};

/* 自动注册/注销PCI驱动 */
module_pci_driver(dma_exp_driver);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("pcie_dma module");
MODULE_LICENSE("GPL v2");