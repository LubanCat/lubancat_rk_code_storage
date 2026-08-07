/*
 * PCIe 中断机制实验驱动
 *
 * 参考rtw88 pci.c的中断实现：
 *   rtw_pci_request_irq()        -> 中断向量申请
 *   rtw_pci_interrupt_handler()  -> 硬中断（上半部）
 *   rtw_pci_interrupt_threadfn() -> 线程中断（下半部）
 *   rtw_pci_irq_recognized()     -> 中断状态读取与清除
 *   rtw_pci_enable_interrupt()   -> 中断使能
 *   rtw_pci_disable_interrupt()  -> 中断禁用
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/pci_regs.h>
#include <linux/version.h>

#define DRV_NAME    "pcie_irq"  /* 驱动名称 */
#define DRV_VERSION "1.0"       /* 驱动版本号 */

/* 以下寄存器地址全部来自rtw88驱动的pci.h */

/* 中断掩码寄存器 */
#define RTK_PCI_HIMR0       0x0B0   /* 中断掩码寄存器0 */
#define RTK_PCI_HISR0       0x0B4   /* 中断状态寄存器0 */
#define RTK_PCI_HIMR1       0x0B8   /* 中断掩码寄存器1 */
#define RTK_PCI_HISR1       0x0BC   /* 中断状态寄存器1 */
#define RTK_PCI_HIMR3       0x10B8  /* 中断掩码寄存器3 */
#define RTK_PCI_HISR3       0x10BC  /* 中断状态寄存器3 */

/* IMR 0 位定义（HIMR0/HISR0） */
#define IMR_ROK             BIT(0)    /* 接收DMA完成 */
#define IMR_RDU             BIT(1)    /* 接收描述符不可用 */
#define IMR_VODOK           BIT(2)    /* VO队列发送DMA完成 */
#define IMR_VIDOK           BIT(3)    /* VI队列发送DMA完成 */
#define IMR_BEDOK           BIT(4)    /* BE队列发送DMA完成 */
#define IMR_BKDOK           BIT(5)    /* BK队列发送DMA完成 */
#define IMR_MGNTDOK         BIT(6)    /* 管理队列发送DMA完成 */
#define IMR_HIGHDOK         BIT(7)    /* 高优先级队列DMA完成 */
#define IMR_CPWM            BIT(8)    /* 电源模式切换 */
#define IMR_CPWM2           BIT(9)    /* 电源模式切换2 */
#define IMR_C2HCMD          BIT(10)   /* 固件C2H命令中断 */
#define IMR_BCNDMAINT_E     BIT(14)   /* Beacon DMA中断 */

/* IMR 1 位定义（HIMR1/HISR1） */
#define IMR_TXFOVW          BIT(9)    /* TX FIFO溢出 */

/* IMR 3 位定义（HIMR3/HISR3） */
#define IMR_H2CDOK          BIT(16)   /* H2C队列DMA完成 */


/* 参考pci.c：rtw_pci_init()中irq_mask的定义
*   rtwpci->irq_mask[0] = IMR_HIGHDOK | IMR_MGNTDOK |
*                         IMR_BKDOK | IMR_BEDOK |
*                         IMR_VIDOK | IMR_VODOK |
*                         IMR_ROK | IMR_BCNDMAINT_E |
*                         IMR_C2HCMD;
*   rtwpci->irq_mask[1] = IMR_TXFOVW;
*   rtwpci->irq_mask[3] = IMR_H2CDOK;
*/
#define EXP_IRQ_MASK0  (IMR_HIGHDOK | IMR_MGNTDOK | \
                        IMR_BKDOK | IMR_BEDOK | \
                        IMR_VIDOK | IMR_VODOK | \
                        IMR_ROK | IMR_BCNDMAINT_E | \
                        IMR_C2HCMD)
#define EXP_IRQ_MASK1  (IMR_TXFOVW)
#define EXP_IRQ_MASK3  (IMR_H2CDOK)


/* 设备私有数据结构体  */
struct irq_exp_dev {
    struct pci_dev *pdev;   /* PCI设备指针，用于访问配置空间 */
    void __iomem *bar_mmap; /* BAR2 MMIO映射后的虚拟地址 */
    size_t bar_size;        /* BAR2 空间大小 */

    /* 中断相关字段 */
    int irq_vector;         /* 分配到的中断向量号 */
    bool irq_registered;    /* 中断处理函数是否已注册 */
    bool irq_enabled;       /* 中断是否已使能 */
    atomic_t irq_count;     /* 中断触发总计数 */
    u32 last_hisr0;         /* 最后一次中断时的HISR0值 */
    u32 last_hisr1;         /* 最后一次中断时的HISR1值 */
    u32 last_hisr3;         /* 最后一次中断时的HISR3值 */
    spinlock_t irq_lock;    /* 中断操作自旋锁 */
    struct mutex lock;      /* 互斥锁 */
};

/*
 * 硬中断处理函数（上半部）
 *
 * 参考pci.c：rtw_pci_interrupt_handler()
 */
static irqreturn_t irq_exp_hard_handler(int irq, void *dev_id)
{
    /* 获取设备私有数据 */
    struct irq_exp_dev *edev = dev_id;

    /*
     * 立即关闭所有中断掩码
     *
     * 参考pci.c：rtw_pci_disable_interrupt()
     *   rtw_write32(rtwdev, RTK_PCI_HIMR0, 0);
     *   rtw_write32(rtwdev, RTK_PCI_HIMR1, 0);
     *   rtw_write32(rtwdev, RTK_PCI_HIMR3, 0);
     */
    writel(0, edev->bar_mmap + RTK_PCI_HIMR0);      /* 关闭中断掩码0 */
    writel(0, edev->bar_mmap + RTK_PCI_HIMR1);      /* 关闭中断掩码1 */
    writel(0, edev->bar_mmap + RTK_PCI_HIMR3);      /* 关闭中断掩码3 */

    /* 唤醒中断线程执行实际处理 */
    return IRQ_WAKE_THREAD;
}

/*
 * 中断线程处理函数（下半部）
 *
 * 参考 pci.c：rtw_pci_interrupt_threadfn()
 */
static irqreturn_t irq_exp_thread_handler(int irq, void *dev_id)
{
    /* 获取设备私有数据 */
    struct irq_exp_dev *edev = dev_id;
    /* 保存中断标志 */
    unsigned long flags;
    /* 中断状态寄存器值 */
    u32 hisr0, hisr1, hisr3;

    /* 获取自旋锁并保存中断状态 */
    spin_lock_irqsave(&edev->irq_lock, flags);

    /*
     * 步骤1：读取中断状态
     *
     * 参考 pci.c：rtw_pci_irq_recognized()
     *   irq_status[0] = rtw_read32(rtwdev, RTK_PCI_HISR0);
     *   irq_status[1] = rtw_read32(rtwdev, RTK_PCI_HISR1);
     *   irq_status[3] = rtw_read32(rtwdev, RTK_PCI_HISR3);
     */
    hisr0 = readl(edev->bar_mmap + RTK_PCI_HISR0);      /* 读取中断状态0 */
    hisr1 = readl(edev->bar_mmap + RTK_PCI_HISR1);      /* 读取中断状态1 */
    hisr3 = readl(edev->bar_mmap + RTK_PCI_HISR3);      /* 读取中断状态3 */

    edev->last_hisr0 = hisr0;                           /* 保存HISR0供sysfs查询 */
    edev->last_hisr1 = hisr1;                           /* 保存HISR1供sysfs查询 */
    edev->last_hisr3 = hisr3;                           /* 保存HISR3供sysfs查询 */

    /*
     * 步骤2：W1C清除中断状态
     *
     * 参考 pci.c：rtw_pci_irq_recognized()
     *   rtw_write32(rtwdev, RTK_PCI_HISR0, irq_status[0]);
     *   rtw_write32(rtwdev, RTK_PCI_HISR1, irq_status[1]);
     *   rtw_write32(rtwdev, RTK_PCI_HISR3, irq_status[3]);
     *
     * W1C即向某位写1清除该位，写0无影响
     */
    writel(hisr0, edev->bar_mmap + RTK_PCI_HISR0);       /* W1C清除中断状态0 */
    writel(hisr1, edev->bar_mmap + RTK_PCI_HISR1);       /* W1C清除中断状态1 */
    writel(hisr3, edev->bar_mmap + RTK_PCI_HISR3);       /* W1C清除中断状态3 */

    /* 递增中断计数 */
    atomic_inc(&edev->irq_count);

    /*
     * 步骤3：解析中断源
     *
     * 参考 pci.c：rtw_pci_interrupt_threadfn()
     *   if (irq_status[0] & IMR_MGNTDOK)
     *       rtw_pci_tx_isr(rtwdev, rtwpci, RTW_TX_QUEUE_MGMT);
     *   ...
     *   if (irq_status[0] & IMR_ROK) {
     *       rtw_pci_rx_isr(rtwdev);
     *       rx = true;
     *   }
     */
    if (atomic_read(&edev->irq_count) <= 20) {
        pr_info("%s: IRQ #%d  HISR0=0x%08X HISR1=0x%08X HISR3=0x%08X\n",
                DRV_NAME, atomic_read(&edev->irq_count),
                hisr0, hisr1, hisr3);

        /* HISR0 位解析 */
        if (hisr0 & IMR_ROK)
            pr_info("  -> HISR0 BIT(0)  IMR_ROK           接收DMA完成\n");
        if (hisr0 & IMR_RDU)
            pr_info("  -> HISR0 BIT(1)  IMR_RDU           接收描述符不可用\n");
        if (hisr0 & IMR_VODOK)
            pr_info("  -> HISR0 BIT(2)  IMR_VODOK         VO队列发送完成\n");
        if (hisr0 & IMR_VIDOK)
            pr_info("  -> HISR0 BIT(3)  IMR_VIDOK         VI队列发送完成\n");
        if (hisr0 & IMR_BEDOK)
            pr_info("  -> HISR0 BIT(4)  IMR_BEDOK         BE队列发送完成\n");
        if (hisr0 & IMR_BKDOK)
            pr_info("  -> HISR0 BIT(5)  IMR_BKDOK         BK队列发送完成\n");
        if (hisr0 & IMR_MGNTDOK)
            pr_info("  -> HISR0 BIT(6)  IMR_MGNTDOK       管理队列发送完成\n");
        if (hisr0 & IMR_HIGHDOK)
            pr_info("  -> HISR0 BIT(7)  IMR_HIGHDOK       高优先级发送完成\n");
        if (hisr0 & IMR_CPWM)
            pr_info("  -> HISR0 BIT(8)  IMR_CPWM          电源模式切换\n");
        if (hisr0 & IMR_CPWM2)
            pr_info("  -> HISR0 BIT(9)  IMR_CPWM2         电源模式切换2\n");
        if (hisr0 & IMR_C2HCMD)
            pr_info("  -> HISR0 BIT(10) IMR_C2HCMD        固件C2H命令\n");
        if (hisr0 & BIT(11))
            pr_info("  -> HISR0 BIT(11) IMR_HISR1_IND_INT HISR1中断指示\n");
        if (hisr0 & BIT(12))
            pr_info("  -> HISR0 BIT(12) IMR_ATIMEND       ATIM结束\n");
        if (hisr0 & IMR_BCNDMAINT_E)
            pr_info("  -> HISR0 BIT(14) IMR_BCNDMAINT_E   Beacon DMA中断\n");
        if (hisr0 & BIT(15))
            pr_info("  -> HISR0 BIT(15) IMR_HSISR_IND_ON  HSISR中断指示\n");
        if (hisr0 & BIT(16))
            pr_info("  -> HISR0 BIT(16) IMR_BCNDOK0       Beacon DMA完成0\n");
        if (hisr0 & BIT(20))
            pr_info("  -> HISR0 BIT(20) IMR_BCNDMAINT0    Beacon DMA中断0\n");
        if (hisr0 & BIT(24))
            pr_info("  -> HISR0 BIT(24) IMR_TSF_BIT32     TSF bit32翻转\n");
        if (hisr0 & BIT(25))
            pr_info("  -> HISR0 BIT(25) IMR_TBDOK         TBDOK\n");
        if (hisr0 & BIT(26))
            pr_info("  -> HISR0 BIT(26) IMR_TBDER         TBDER\n");
        if (hisr0 & BIT(27))
            pr_info("  -> HISR0 BIT(27) IMR_GTINT3        GTINT3\n");
        if (hisr0 & BIT(28))
            pr_info("  -> HISR0 BIT(28) IMR_GTINT4        GTINT4\n");
        if (hisr0 & BIT(29))
            pr_info("  -> HISR0 BIT(29) IMR_PSTIMEOUT     省电超时\n");
        if (hisr0 & BIT(30))
            pr_info("  -> HISR0 BIT(30) IMR_TIMER1        定时器1\n");
        if (hisr0 & BIT(31))
            pr_info("  -> HISR0 BIT(31) IMR_TIMER2        定时器2\n");

        /* HISR1 位解析 */
        if (hisr1 & BIT(0))
            pr_info("  -> HISR1 BIT(0)  保留位\n");
        if (hisr1 & BIT(1))
            pr_info("  -> HISR1 BIT(1)  IMR_CPUMGQ_TX_TIMER CPU管理队列TX定时器\n");
        if (hisr1 & BIT(2))
            pr_info("  -> HISR1 BIT(2)  IMR_PS_TIMER_A 省电定时器A\n");
        if (hisr1 & BIT(3))
            pr_info("  -> HISR1 BIT(3)  IMR_PS_TIMER_B 省电定时器B\n");
        if (hisr1 & BIT(4))
            pr_info("  -> HISR1 BIT(4)  IMR_PS_TIMER_C 省电定时器C\n");
        if (hisr1 & BIT(5))
            pr_info("  -> HISR1 BIT(5)  IMR_CPU_MGQ_TXDONE CPU管理队列TX完成\n");
        if (hisr1 & BIT(8))
            pr_info("  -> HISR1 BIT(8)  IMR_RXFOVW RX FIFO溢出\n");
        if (hisr1 & IMR_TXFOVW)
            pr_info("  -> HISR1 BIT(9)  IMR_TXFOVW TX FIFO溢出\n");
        if (hisr1 & BIT(10))
            pr_info("  -> HISR1 BIT(10) IMR_RXERR RX错误\n");
        if (hisr1 & BIT(11))
            pr_info("  -> HISR1 BIT(11) IMR_TXERR TX错误\n");
        if (hisr1 & BIT(12))
            pr_info("  -> HISR1 BIT(12) IMR_ATIMEND ATIM结束\n");
        if (hisr1 & BIT(29))
            pr_info("  -> HISR1 BIT(29) IMR_BTON_STS_UPDATE BT状态更新\n");
        if (hisr1 & BIT(30))
            pr_info("  -> HISR1 BIT(30) IMR_TXFIFO_TH_INT TX FIFO阈值中断\n");

        /* HISR3 位解析 */
        if (hisr3 & IMR_H2CDOK)
            pr_info("  -> HISR3 BIT(16) IMR_H2CDOK H2C DMA完成\n");
    }

    /*
     * 步骤4：重新使能中断
     *
     * 参考pci.c：rtw_pci_interrupt_threadfn()
     *   if (rtwpci->running)
     *       rtw_pci_enable_interrupt(rtwdev, rtwpci, rx);
     */

    writel(EXP_IRQ_MASK0, edev->bar_mmap + RTK_PCI_HIMR0); /* 使能全部HIMR0中断 */
    writel(EXP_IRQ_MASK1, edev->bar_mmap + RTK_PCI_HIMR1); /* 使能TX FIFO溢出 */
    writel(EXP_IRQ_MASK3, edev->bar_mmap + RTK_PCI_HIMR3); /* 使能H2C DMA完成 */

    /* 释放自旋锁并恢复中断状态 */
    spin_unlock_irqrestore(&edev->irq_lock, flags);

    /* 返回中断已处理 */
    return IRQ_HANDLED;
}

/* 
 * 中断子系统初始化
 *
 * 参考pci.c：rtw_pci_request_irq() + rtw_pci_init()
  */
static int irq_exp_init(struct irq_exp_dev *edev)
{
    /* 获取PCI设备指针 */
    struct pci_dev *pdev = edev->pdev;
    /* 返回值 */
    int ret;

    pr_info("--------- 中断子系统初始化 ---------\n");

    /*
     * 步骤1：申请中断向量
     *
     * 参考pci.c：rtw_pci_request_irq()
     *   unsigned int flags = PCI_IRQ_LEGACY;  // 旧内核
     *   unsigned int flags = PCI_IRQ_INTX;    // 新内核 >= 6.10
     *   if (!rtw_disable_msi)
     *       flags |= PCI_IRQ_MSI;
     *   ret = pci_alloc_irq_vectors(pdev, 1, 1, flags);
     *
     * 优先MSI，回退INTx
     */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 10, 0))         /* 内核版本判断 */
    ret = pci_alloc_irq_vectors(pdev, 1, 1,                  /* 申请1个中断向量 */
                                PCI_IRQ_MSI | PCI_IRQ_INTX); /* MSI优先，INTx回退 */
#else                                                        /* 旧内核 */
    ret = pci_alloc_irq_vectors(pdev, 1, 1,                  /* 申请1个中断向量 */
                                PCI_IRQ_MSI | PCI_IRQ_LEGACY); /* MSI优先，LEGACY回退 */
#endif
    if (ret < 0) {                                           /* 申请失败 */
        pr_err("中断向量申请失败: %d\n", ret);
        return ret;
    }

    edev->irq_vector = pci_irq_vector(pdev, 0);              /* 获取第0个向量的IRQ号 */
    pr_info("中断向量申请成功: IRQ=%d\n", edev->irq_vector); /* 打印IRQ号 */

    /*
     * 步骤2：注册线程化中断
     *
     * 参考pci.c：rtw_pci_request_irq()
     *   ret = devm_request_threaded_irq(rtwdev->dev, pdev->irq,
     *                                   rtw_pci_interrupt_handler,
     *                                   rtw_pci_interrupt_threadfn,
     *                                   IRQF_SHARED, KBUILD_MODNAME,
     *                                   rtwdev);
     *
     * 硬中断：irq_exp_hard_handler
     * 线程：irq_exp_thread_handler
     */
    ret = request_threaded_irq(edev->irq_vector,            /* IRQ号 */
                               irq_exp_hard_handler,        /* 硬中断处理函数 */
                               irq_exp_thread_handler,      /* 线程处理函数 */
                               IRQF_SHARED,                 /* 允许共享中断 */
                               DRV_NAME,                    /* 中断名称 */
                               edev);                       /* 传递给handler的参数 */
    if (ret) {                                              /* 注册失败 */
        pr_err("中断注册失败: %d\n", ret);                  /* 打印错误 */
        pci_free_irq_vectors(pdev);                         /* 释放中断向量 */
        return ret;                                         /* 返回错误码 */
    }
    /* 标记中断已注册 */
    edev->irq_registered = true;
    pr_info("线程化中断注册成功\n");

    /* 打印掩码值 */
    pr_info("默认中断掩码: HIMR0=0x%08X HIMR1=0x%08X HIMR3=0x%08X\n",
            (u32)EXP_IRQ_MASK0, (u32)EXP_IRQ_MASK1, (u32)EXP_IRQ_MASK3);

    /*
     * 步骤3：使能中断
     *
     * 参考pci.c：rtw_pci_enable_interrupt()
     *   rtw_write32(rtwdev, RTK_PCI_HIMR0, rtwpci->irq_mask[0]);
     *   rtw_write32(rtwdev, RTK_PCI_HIMR1, rtwpci->irq_mask[1]);
     *   rtw_write32(rtwdev, RTK_PCI_HIMR3, rtwpci->irq_mask[3]);
     */
    writel(EXP_IRQ_MASK0, edev->bar_mmap + RTK_PCI_HIMR0); /* 使能全部HIMR0中断 */
    writel(EXP_IRQ_MASK1, edev->bar_mmap + RTK_PCI_HIMR1); /* 使能TX FIFO溢出 */
    writel(EXP_IRQ_MASK3, edev->bar_mmap + RTK_PCI_HIMR3); /* 使能H2C DMA完成 */
    /* 标记中断已使能 */
    edev->irq_enabled = true;
    /* 打印提示 */
    pr_info("中断已使能，等待设备触发...\n");

    /*
     * 设备未加载firmware，可能不会产生中断，并且前后还有大量必需步骤才能真正实现
     */

    return 0;
}

/* 中断子系统清理 */
static void irq_exp_cleanup(struct irq_exp_dev *edev)
{
    /* 获取PCI设备指针 */
    struct pci_dev *pdev = edev->pdev;

    /* 未注册则直接返回 */
    if (!edev->irq_registered)
        return;
    
    /* BAR是否映射 */
    if (edev->bar_mmap) {
        writel(0, edev->bar_mmap + RTK_PCI_HIMR0);          /* 关闭中断掩码0 */
        writel(0, edev->bar_mmap + RTK_PCI_HIMR1);          /* 关闭中断掩码1 */
        writel(0, edev->bar_mmap + RTK_PCI_HIMR3);          /* 关闭中断掩码3 */
    }
    /* 标记中断已禁用 */
    edev->irq_enabled = false;

    /* 等待正在处理的中断完成 */
    synchronize_irq(edev->irq_vector);

    /* 释放中断处理函数 */
    free_irq(edev->irq_vector, edev);
    /* 标记中断未注册 */
    edev->irq_registered = false;

    /* 释放中断向量 */
    pci_free_irq_vectors(pdev);

    /* 打印释放信息 */
    pr_info("%s:中断资源已释放，总中断次数: %d\n",
            DRV_NAME, atomic_read(&edev->irq_count));
}

/* sysfs属性接口：显示中断状态信息 */
static ssize_t irq_info_show(struct device *dev,
                             struct device_attribute *attr,
                             char *buf)
{
    /* 获取设备私有数据 */
    struct irq_exp_dev *edev = dev_get_drvdata(dev);
    /* 已写入长度 */
    int len = 0;

    /* 空指针检查 */
    if (!edev)
        return -ENODEV;

    len += sprintf(buf + len, "--------- PCI中断信息 ---------\n");    /* 标题 */
    len += sprintf(buf + len, "中断向量:   %d\n", edev->irq_vector);        /* IRQ号 */
    len += sprintf(buf + len, "已注册:     %s\n",            /* 注册状态 */
                   edev->irq_registered ? "Yes" : "No");
    len += sprintf(buf + len, "已使能:     %s\n",            /* 使能状态 */
                   edev->irq_enabled ? "Yes" : "No");
    len += sprintf(buf + len, "中断计数:   %d\n",            /* 中断计数 */
                   atomic_read(&edev->irq_count));
    len += sprintf(buf + len, "\n");                        /* 空行 */
    len += sprintf(buf + len, "最后中断状态:\n");           /* 最后中断状态标题 */
    len += sprintf(buf + len, "  HISR0 = 0x%08X\n", edev->last_hisr0); /* HISR0 */
    len += sprintf(buf + len, "  HISR1 = 0x%08X\n", edev->last_hisr1); /* HISR1 */
    len += sprintf(buf + len, "  HISR3 = 0x%08X\n", edev->last_hisr3); /* HISR3 */

    /* BAR已映射时读取实时值 */
    if (edev->bar_mmap) {
        len += sprintf(buf + len, "\n当前寄存器值:\n");     /* 实时寄存器标题 */
        len += sprintf(buf + len, "  HISR0 [0x%04X] = 0x%08X\n", /* 实时HISR0 */
                       RTK_PCI_HISR0,
                       readl(edev->bar_mmap + RTK_PCI_HISR0));
        len += sprintf(buf + len, "  HIMR0 [0x%04X] = 0x%08X\n", /* 实时HIMR0 */
                       RTK_PCI_HIMR0,
                       readl(edev->bar_mmap + RTK_PCI_HIMR0));
        len += sprintf(buf + len, "  HISR1 [0x%04X] = 0x%08X\n", /* 实时HISR1 */
                       RTK_PCI_HISR1,
                       readl(edev->bar_mmap + RTK_PCI_HISR1));
        len += sprintf(buf + len, "  HIMR1 [0x%04X] = 0x%08X\n", /* 实时HIMR1 */
                       RTK_PCI_HIMR1,
                       readl(edev->bar_mmap + RTK_PCI_HIMR1));
    }

    /* 返回写入的总字节数 */
    return len;
}

/* 使能/禁用中断 */
static ssize_t irq_enable_store(struct device *dev,
                                struct device_attribute *attr,
                                const char *buf,
                                size_t count)
{
    /* 获取设备私有数据 */
    struct irq_exp_dev *edev = dev_get_drvdata(dev);
    /* 使能/禁用标志 */
    int enable;

    /* 空指针检查 */
    if (!edev || !edev->bar_mmap)
        return -ENODEV;

    /* 解析用户输入 */
    if (kstrtoint(buf, 10, &enable))
        return -EINVAL;
    
    if (enable) { /* 使能中断 */
        writel(EXP_IRQ_MASK0, edev->bar_mmap + RTK_PCI_HIMR0); /* 使能全部HIMR0中断 */
        writel(EXP_IRQ_MASK1, edev->bar_mmap + RTK_PCI_HIMR1); /* 使能TX FIFO溢出 */
        writel(EXP_IRQ_MASK3, edev->bar_mmap + RTK_PCI_HIMR3); /* 使能H2C DMA完成 */
        edev->irq_enabled = true;
        pr_info("%s: 全部中断已使能 (HIMR0=0x%08X)\n",
                DRV_NAME, (u32)EXP_IRQ_MASK0);
    } else {     /* 禁用中断 */
        /*
         * 参考pci.c：rtw_pci_disable_interrupt()
         *   rtw_write32(rtwdev, RTK_PCI_HIMR0, 0);
         */
        writel(0, edev->bar_mmap + RTK_PCI_HIMR0);          /* 关闭掩码0 */
        writel(0, edev->bar_mmap + RTK_PCI_HIMR1);          /* 关闭掩码1 */
        writel(0, edev->bar_mmap + RTK_PCI_HIMR3);          /* 关闭掩码3 */
        edev->irq_enabled = false;                          /* 标记已禁用 */
        pr_info("%s: 中断已禁用\n", DRV_NAME);              /* 打印禁用信息 */
    }

    /* 返回写入的总字节数 */
    return count;
}

/*
 * 手动触发中断
 *
 * 原理：将HIMR设置为HISR中已置位的位
 * 使HISR & HIMR从0变为非0，硬件触发MSI中断。
 *
 * 触发后的完整流程：
 *   1. 硬件检测到HISR & HIMR != 0 -> 触发MSI
 *   2. 硬中断irq_exp_hard_handler：写HIMR=0 关闭中断
 *   3. 线程irq_exp_thread_handler：读HISR -> W1C清除 -> 打印中断信息 -> 重新使能
 *
 * 额外说明：
 *   上电或卸载内核自带网卡驱动时残留的HISR位，如IMR_CPWM、IMR_BCNDMAINT0
 *   在无firmware时不会再次置位，所以只触发一次。
 */
static ssize_t irq_trigger_store(struct device *dev,
                                 struct device_attribute *attr,
                                 const char *buf,
                                 size_t count)
{
    /* 获取设备私有数据 */
    struct irq_exp_dev *edev = dev_get_drvdata(dev);
    /* 中断状态寄存器值 */
    u32 hisr0, hisr1, hisr3;
    /* 触发标志 */
    int trigger;

    /* 空指针检查 */
    if (!edev || !edev->bar_mmap)
        return -ENODEV;     /* 返回无设备错误 */

    /* 解析用户输入 */
    if (kstrtoint(buf, 10, &trigger))
        return -EINVAL;    /* 解析失败返回错误 */

    /* 只接受写入1 */
    if (trigger != 1)
        return count;      /* 忽略其他值 */

    /*
     * 步骤1：读取当前HISR状态，找到已置位的位
     */
    hisr0 = readl(edev->bar_mmap + RTK_PCI_HISR0);  /* 读取HISR0 */
    hisr1 = readl(edev->bar_mmap + RTK_PCI_HISR1);  /* 读取HISR1 */
    hisr3 = readl(edev->bar_mmap + RTK_PCI_HISR3);  /* 读取HISR3 */

    /* 没有已置位的中断 */
    if (!hisr0 && !hisr1 && !hisr3) {
        pr_info("%s: HISR中没有已置位的中断位，无法触发\n", DRV_NAME);
        pr_info("  当前HISR0=0x00000000 HISR1=0x00000000 HISR3=0x00000000\n");
        return count;
    }

    pr_info("%s: --------- 手动触发中断 ---------\n", DRV_NAME);
    pr_info("  当前 HISR0=0x%08X HISR1=0x%08X HISR3=0x%08X\n", /* 打印当前状态 */
            hisr0, hisr1, hisr3);

    /*
     * 步骤2：将HIMR设置为HISR中已置位的位
     *
     * 此时HISR & HIMR != 0，硬件触发MSI中断
     *
     * 参考pci.c的中断使能：
     *   rtw_write32(rtwdev, RTK_PCI_HIMR0, rtwpci->irq_mask[0]);
     *   rtw_write32(rtwdev, RTK_PCI_HIMR1, rtwpci->irq_mask[1]);
     *   rtw_write32(rtwdev, RTK_PCI_HIMR3, rtwpci->irq_mask[3]);
     *
     * 这里我们用HISR值代替irq_mask，确保HISR & HIMR != 0
     */
    writel(hisr0, edev->bar_mmap + RTK_PCI_HIMR0);          /* 写入HIMR0 */
    writel(hisr1, edev->bar_mmap + RTK_PCI_HIMR1);          /* 写入HIMR1 */
    writel(hisr3, edev->bar_mmap + RTK_PCI_HIMR3);          /* 写入HIMR3 */

    pr_info("已写入 HIMR0=0x%08X HIMR1=0x%08X HIMR3=0x%08X\n", /* 打印写入值 */
            hisr0, hisr1, hisr3);
    pr_info("等待硬件触发MSI中断...\n");                    /* 打印等待提示 */

    /* 返回写入的总字节数 */
    return count;
}

/* sysfs属性定义 */
static DEVICE_ATTR(irq_info,    0444, irq_info_show,    NULL); /* 中断信息，只读 */
static DEVICE_ATTR(irq_enable,  0200, NULL,             irq_enable_store);  /* 使能/禁用，只写 */
static DEVICE_ATTR(irq_trigger, 0200, NULL,             irq_trigger_store); /* 手动触发，只写 */

/* 属性数组 */
static struct attribute *irq_exp_attrs[] = {
    &dev_attr_irq_info.attr,               /* irq_info属性 */
    &dev_attr_irq_enable.attr,             /* irq_enable属性 */
    &dev_attr_irq_trigger.attr,            /* irq_trigger 属性 */
    NULL,                                  /* 数组结束标记 */
};

/* 属性组定义 */
static const struct attribute_group irq_exp_group = {
    .attrs = irq_exp_attrs,                /* 指向属性数组 */
};

/*
 * 设备探测函数
 *
 * 参考 pci.c：rtw_pci_probe()
 */
static int irq_exp_probe(struct pci_dev *pdev,
                         const struct pci_device_id *id)
{
    /* 设备私有数据指针 */
    struct irq_exp_dev *edev;
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
    /* 初始化自旋锁 */
    spin_lock_init(&edev->irq_lock);
    /* 中断计数初始化为0 */
    atomic_set(&edev->irq_count, 0);
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
        pr_err("BAR2大小为 0\n");
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

    /* 初始化中断子系统 */
    ret = irq_exp_init(edev);
    if (ret) {
        pr_err("中断初始化失败: %d\n", ret);
        goto err_unmap;
    }

    /* 创建sysfs属性组 */
    ret = devm_device_add_group(&pdev->dev, &irq_exp_group);
    if (ret)
        goto err_irq_cleanup;

    pr_info("%s: 设备 %s 初始化完成\n",
            DRV_NAME, pci_name(pdev));
    return 0;

err_irq_cleanup:
    /* 释放中断资源 */
    irq_exp_cleanup(edev);
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
static void irq_exp_remove(struct pci_dev *pdev)
{
    /* 获取私有数据 */
    struct irq_exp_dev *edev = pci_get_drvdata(pdev);

    /* 空指针检查 */
    if (!edev)
        return;
    
    /* 移除sysfs属性组 */
    device_remove_group(&pdev->dev, &irq_exp_group);
    /* 释放中断资源 */
    irq_exp_cleanup(edev);

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

    pr_info("%s:设备 %s 已移除\n", DRV_NAME, pci_name(pdev));
}

/* 系统挂起回调 */
static int irq_exp_suspend(struct device *dev)
{
    /* 转换为PCI设备 */
    struct pci_dev *pdev = to_pci_dev(dev);
    /* 获取私有数据 */
    struct irq_exp_dev *edev = pci_get_drvdata(pdev);

    /* 打印挂起信息 */
    pr_info("%s: suspend\n", DRV_NAME);

    /* 空指针检查 */
    if (!edev)
        return 0;
    
    /* BAR已映射且中断已使能 */
    if (edev->bar_mmap && edev->irq_enabled) {
        writel(0, edev->bar_mmap + RTK_PCI_HIMR0);          /* 关闭中断掩码0 */
        writel(0, edev->bar_mmap + RTK_PCI_HIMR1);          /* 关闭中断掩码1 */
        writel(0, edev->bar_mmap + RTK_PCI_HIMR3);          /* 关闭中断掩码3 */
    }

    /* 保存PCI配置空间状态 */
    pci_save_state(pdev);
    /* 禁用PCI设备 */
    pci_disable_device(pdev);

    return 0;
}

/* 系统恢复回调 */
static int irq_exp_resume(struct device *dev)
{
    /* 转换为PCI设备 */
    struct pci_dev *pdev = to_pci_dev(dev);
    /* 获取私有数据 */
    struct irq_exp_dev *edev = pci_get_drvdata(pdev);
    /* 返回值 */
    int ret;

    /* 打印恢复信息 */
    pr_info("%s: resume\n", DRV_NAME);

    /* 空指针检查 */
    if (!edev)
        return 0;

    /* 恢复PCI配置空间状态 */
    pci_restore_state(pdev);

    /* 重新使能PCI设备 */
    ret = pci_enable_device(pdev);
    if (ret) {
        pr_err("resume: pci_enable_device 失败: %d\n", ret);
        return ret;
    }
     /* 重新设置总线主控 */
    pci_set_master(pdev);

    /* BAR已映射且中断已注册 */
    if (edev->bar_mmap && edev->irq_registered) {
        writel(EXP_IRQ_MASK0, edev->bar_mmap + RTK_PCI_HIMR0); /* 使能全部HIMR0中断 */
        writel(EXP_IRQ_MASK1, edev->bar_mmap + RTK_PCI_HIMR1); /* 使能TX FIFO溢出 */
        writel(EXP_IRQ_MASK3, edev->bar_mmap + RTK_PCI_HIMR3); /* 使能H2C DMA完成 */
        edev->irq_enabled = true;                              /* 标记中断已使能 */
    }

    return 0;
}

/* 使用dev_pm_ops接口定义电源管理回调 */
static SIMPLE_DEV_PM_OPS(irq_exp_pm_ops,    /* PM操作结构体名 */
                         irq_exp_suspend,   /* 挂起回调 */
                         irq_exp_resume);   /* 恢复回调 */

/* PCI 设备 ID 匹配表 */
static const struct pci_device_id irq_exp_ids[] = {
    { PCI_DEVICE(0x10EC, 0x8179) },  /* RTL8188EE */
    { PCI_DEVICE(0x10EC, 0xC821) },  /* RTL8821CE */
    { PCI_DEVICE(0x10EC, 0xC822) },  /* RTL8822CE */
    { PCI_DEVICE(0x10EC, 0xC82F) },  /* RTL8822CE */
    { PCI_DEVICE(0x10EC, 0xb852) },  /* RTL8852BE */
    { PCI_DEVICE(0x10EC, 0xb85b) },  /* RTL8852BE */
    { }
};
/* 导出设备ID表到用户空间 */
MODULE_DEVICE_TABLE(pci, irq_exp_ids);

/* PCI驱动结构体 */
static struct pci_driver irq_exp_driver = {
    .name      = DRV_NAME,             /* 驱动名称 */
    .id_table  = irq_exp_ids,          /* 设备匹配表 */
    .probe     = irq_exp_probe,        /* 设备匹配成功后的初始化入口 */
    .remove    = irq_exp_remove,       /* 设备移除时的清理入口 */
    .driver.pm = &irq_exp_pm_ops,      /* 电源管理操作 */
};

/* 自动注册/注销PCI驱动 */
module_pci_driver(irq_exp_driver);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("pcie_irq module");
MODULE_LICENSE("GPL v2");