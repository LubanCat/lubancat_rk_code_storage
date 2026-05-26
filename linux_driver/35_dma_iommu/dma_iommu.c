#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_platform.h> 
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/mutex.h>

/* 设备名称 */
#define DEVICE_NAME "dma_iommu"
/* DMA缓冲区大小设置为4KB */
#define DMA_BUFFER_SIZE (4 * 1024)

/* DMA IOMMU私有数据结构体 */
struct dma_iommu_priv {
    dma_addr_t dma_addr;             /* DMA地址 */
    void *virt_addr;                 /* 虚拟地址 */
    size_t size;                     /* 缓冲区大小 */
    size_t actual_len;               /* 记录有效数据长度 */
    struct mutex lock;               /* 互斥体 */
    struct cdev cdev;                /* 字符设备 */
    struct device *dev;              /* 设备指针 */
    dev_t devt;                      /* 设备号 */
    struct class *class;             /* 设备类指针 */
};

/* 打开设备函数 */
static int dma_iommu_open(struct inode *inode, struct file *file)
{
    /* 获取私有数据 */
    struct dma_iommu_priv *priv = container_of(inode->i_cdev, struct dma_iommu_priv, cdev); 
    /* 保存私有数据到文件指针 */
    file->private_data = priv;

    return 0;
}

/* 读取数据函数 */
static ssize_t dma_iommu_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    /* 获取私有数据 */
    struct dma_iommu_priv *priv = file->private_data;
    /* 剩余可读数据长度 */
    size_t remaining;
    /* 本次读取长度 */
    size_t len;
    
    /* 获取互斥锁 */
    mutex_lock(&priv->lock);
    
    /* 没有有效数据或偏移已读到末尾直接返回 */
    if (priv->actual_len == 0 || *ppos >= priv->actual_len)
    {
        mutex_unlock(&priv->lock);
        return 0;
    }
    
    /* 获取剩余可读字节 */
    remaining = priv->actual_len - *ppos;
    /* 限制本次读取数据长度不超过剩余、也不超过用户请求 */
    len = min(count, remaining);
    
    /* 将数据从内核空间复制到用户空间 */
    if (copy_to_user(buf, (char *)priv->virt_addr + *ppos, len))
    {
        mutex_unlock(&priv->lock);     /* 释放互斥锁 */
        return -EFAULT;                /* 返回错误 */
    }
    
    /* 更新文件偏移量 */
    *ppos += len;
    
    /* 释放互斥锁 */
    mutex_unlock(&priv->lock);
    
    /* 返回读取的字节数 */
    return len;
}

/* 写入数据函数 */
static ssize_t dma_iommu_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    /* 获取私有数据 */
    struct dma_iommu_priv *priv = file->private_data;
    /* 写入长度 */
    size_t len;
    
    /* 获取互斥锁 */
    mutex_lock(&priv->lock);
    
    /* 限制最大写入，留1字节给字符串结束符 */
    len = min(count, priv->size - 1);

    /* 只清空本次要用的区域 */
    memset(priv->virt_addr, 0, len + 1);
    
    /* 将数据从用户空间复制到内核空间 */
    if (copy_from_user(priv->virt_addr, buf, len))
    {
        mutex_unlock(&priv->lock);     /* 释放互斥锁 */
        return -EFAULT;                /* 返回错误 */
    }
    
    /* 添加字符串结束符 */
    ((char *)priv->virt_addr)[len] = '\0';

    /* 记录当前有效数据长度*/
    priv->actual_len = len;
    
    /* 打印写入信息 */
    dev_info(priv->dev, "Written to DMA memory: \"%s\"\n", (char *)priv->virt_addr);
    
    /* 重置文件偏移量 */
    *ppos = 0;
    
    /* 释放互斥锁 */
    mutex_unlock(&priv->lock);
    
    /* 返回写入的字节数 */
    return len;
}

/* 关闭设备函数 */
static int dma_iommu_release(struct inode *inode, struct file *file)
{
    return 0;
}

/* 字符设备的文件操作结构体 */
static const struct file_operations dma_iommu_fops = {
    .owner = THIS_MODULE,
    .open = dma_iommu_open,
    .read = dma_iommu_read,
    .write = dma_iommu_write,
    .release = dma_iommu_release,
};

/* 设备探测函数 */
static int dma_iommu_probe(struct platform_device *pdev)
{
    /* 私有数据指针 */
    struct dma_iommu_priv *priv;
    /* 设备指针 */
    struct device *dev = &pdev->dev;
    /* 返回值 */
    int ret;

    /* 分配私有数据内存 */
    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    /* 设置缓冲区大小 */
    priv->size = DMA_BUFFER_SIZE;
    /* 初始化有效数据长度为0 */
    priv->actual_len = 0;
    /* 保存设备指针到私有数据 */
    priv->dev = dev;

    /* 初始化互斥锁 */
    mutex_init(&priv->lock);

    /* 设置DMA掩码为64位 */
    ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(64));
    if (ret)
    {
        printk(KERN_WARNING "Cannot set DMA mask to 64 bit, trying 32 bit\n");
        ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));          /* 尝试设置32位DMA掩码 */
        if (ret)
        {
            printk(KERN_ERR "No suitable DMA available\n");
            return ret;
        }
    }

    /* 分配DMA一致性内存 */
    priv->virt_addr = dma_alloc_coherent(dev, priv->size, &priv->dma_addr, GFP_KERNEL);
    if (!priv->virt_addr)
    {
        printk(KERN_ERR "Failed to allocate DMA memory\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "DMA memory allocated:\n");
    printk(KERN_INFO "  CPU Virtual address: 0x%pK\n", priv->virt_addr);  /* 打印内核虚拟地址 */
    printk(KERN_INFO "  Device IOVA address: %pad\n", &priv->dma_addr);   /* 打印DMA地址 */

    /* 清空DMA缓冲区 */
    memset(priv->virt_addr, 0, priv->size);

    /* 创建设备类 */
    priv->class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(priv->class))
    {
        printk(KERN_ERR "Failed to create class\n");
        ret = PTR_ERR(priv->class);
        goto err_free_dma;
    }

    /* 动态分配字符设备号 */
    ret = alloc_chrdev_region(&priv->devt, 0, 1, DEVICE_NAME);
    if (ret)
    {
        printk(KERN_ERR "Failed to allocate char device region\n");
        goto err_destroy_class;
    }

    /* 初始化字符设备 */
    cdev_init(&priv->cdev, &dma_iommu_fops);
    /* 设置设备所有者 */
    priv->cdev.owner = THIS_MODULE;
    
    /* 添加字符设备 */
    ret = cdev_add(&priv->cdev, priv->devt, 1);
    if (ret)
    {
        printk(KERN_ERR "Failed to add cdev\n");
        goto err_unregister_region;
    }
    /* 打印设备号信息 */
    printk(KERN_INFO "char device major=%d, minor=%d\n", MAJOR(priv->devt), MINOR(priv->devt));

    /* 创建设备节点 */
    if (device_create(priv->class, NULL, priv->devt, NULL, DEVICE_NAME) == NULL)
    {
        printk(KERN_ERR "Failed to create device\n");
        goto err_del_cdev;
    }
    /* 保存私有数据到平台设备 */
    platform_set_drvdata(pdev, priv);

    return 0;

err_del_cdev:
    /* 删除字符设备 */
    cdev_del(&priv->cdev);
err_unregister_region:
    /* 释放设备号 */
    unregister_chrdev_region(priv->devt, 1);
err_destroy_class:
    /* 销毁设备类 */
    class_destroy(priv->class);
err_free_dma:
    /* 释放DMA内存 */
    dma_free_coherent(dev, priv->size, priv->virt_addr, priv->dma_addr);
    return ret;
}

/* 设备移除函数 */
static int dma_iommu_remove(struct platform_device *pdev)
{
    /* 获取私有数据 */
    struct dma_iommu_priv *priv = platform_get_drvdata(pdev);
    /* 设备指针 */
    struct device *dev = &pdev->dev;

    device_destroy(priv->class, priv->devt); /* 销毁设备节点 */
    cdev_del(&priv->cdev);                       /* 删除字符设备 */
    unregister_chrdev_region(priv->devt, 1);     /* 注销设备号 */
    class_destroy(priv->class);                  /* 删除字符设备 */

    /* 检查DMA内存是否存在 */
    if (priv->virt_addr)
    {
        dma_free_coherent(dev, priv->size, priv->virt_addr, priv->dma_addr); /* 释放DMA内存 */
        printk(KERN_INFO "DMA memory freed\n");                                 /* 打印释放信息 */
    }

    /* 销毁互斥锁 */
    mutex_destroy(&priv->lock);

    return 0;
}

/* 设备树匹配表 */
static const struct of_device_id dma_iommu_of_match[] = {
    { .compatible = "fire,dma-iommu" },
    { }
};
/* 声明设备树匹配表，供内核自动匹配设备 */
MODULE_DEVICE_TABLE(of, dma_iommu_of_match);

/* 平台驱动结构体 */
static struct platform_driver dma_iommu_driver = {
    .probe = dma_iommu_probe,
    .remove = dma_iommu_remove,
    .driver = {
        .name = "dma-iommu-test",
        .of_match_table = dma_iommu_of_match,
    },
};

module_platform_driver(dma_iommu_driver);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("dma-iommu module");
MODULE_LICENSE("GPL v2");