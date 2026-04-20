#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/fs.h>

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>

/* 定义字符设备的名称 */
#define DEV_NAME    "dts_led"
/* 定义字符设备的数量 */
#define DEV_CNT     1

/* 定义设备号变量 */
static dev_t devno;
/* 定义设备类指针 */
static struct class *class;
/* 定义设备指针 */
static struct device *device;

/* 定义 LED 字符设备结构体 */
struct led_chrdev {
    /* 字符设备结构体 */
    struct cdev dev;
    /* 数据寄存器的虚拟地址，用于设置输出的电压 */
    unsigned int __iomem *va_dr;
    /* 数据方向寄存器的虚拟地址，用于设置输入或者输出 */
    unsigned int __iomem *va_ddr;
    /* 引脚高低位 */
    unsigned int hl_pos;
    /* 引脚偏移量 */
    unsigned int led_pin;
    /* LED 的设备树子节点 */
    struct device_node *device_node;
};
/* 定义 led_chrdev 结构体指针，用于动态分配内存管理 LED 硬件信息 */
static struct led_chrdev *led_cdev;

/* LED 的设备树节点 */
struct device_node *led_device_node;

/* 
 * pdrv_led_open 函数是字符设备的打开操作函数。
 * 当用户空间程序打开该设备节点时，会调用此函数。
 */
static int pdrv_led_open(struct inode *inode, struct file *filp)
{
    /* 通过 inode 中的 i_cdev 成员找到对应的 led_chrdev 结构体 */
    struct led_chrdev *led_cdev = container_of(inode->i_cdev, struct led_chrdev, dev);
    /* 将 led_chrdev 结构体指针存储到文件的私有数据中 */
    filp->private_data = led_cdev;

    /* 打印设备打开信息 */
    printk("pdrv_led open\n");

    return 0;
}

/* 字符设备释放函数 */
static int pdrv_led_release(struct inode *inode, struct file *filp)
{
    printk("pdrv_led release\r\n");
    return 0;
}

/* 
 * pdrv_led_write 函数是字符设备的写操作函数。
 * 当用户空间程序向该设备节点写入数据时，会调用此函数。
 * 功能：根据用户写入的数据控制 LED 灯的亮灭。
 */
static ssize_t pdrv_led_write(struct file *filp, const char __user * buf,
                              size_t count, loff_t * ppos)
{
    unsigned long val = 0;
    unsigned long ret = 0;
    /* 从文件结构体的私有数据中获取 led_chrdev 结构体指针 */
    struct led_chrdev *led_cdev = filp->private_data;

    /* 打印设备写操作信息 */
    printk("pdrv_led write \r\n");

    /* 从用户空间读取一个字符 */
    get_user(ret, buf);
    /* 读取数据寄存器的值 */
    val = ioread32(led_cdev->va_dr);
    if (ret == '0') {
        /* 设置高 16 位的使能位 */
        val |= ((unsigned int)0x1 << (led_cdev->led_pin + 16));
        /* 设置低 16 位的对应引脚输出低电平 */
        val &= ~((unsigned int)0x01 << (led_cdev->led_pin));
    } else {
        /* 设置高 16 位的使能位 */
        val |= ((unsigned int)0x1 << (led_cdev->led_pin + 16));
        /* 设置低 16 位的对应引脚输出高电平 */
        val |= ((unsigned int)0x01 << (led_cdev->led_pin));
    }
    /* 将修改后的值写回到数据寄存器 */
    iowrite32(val, led_cdev->va_dr);
    return count;
}

/* 字符设备读函数 */
static ssize_t pdrv_led_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    return 0;
}

/* 定义字符设备的文件操作结构体 */
static struct file_operations pdrv_led_fops = {
    .owner = THIS_MODULE,
    .open = pdrv_led_open,
    .release = pdrv_led_release,
    .write = pdrv_led_write,
    .read = pdrv_led_read,
};

/* 
 * pdrv_led_probe 函数是平台驱动的探测函数。
 * 功能：提取平台设备或设备树的资源，设置LED引脚为输出模式，并默认输出高电平，完成字符设备的注册和初始化。
 */
static int pdrv_led_probe(struct platform_device *pdev)
{
    /* 定义引脚偏移变量 */
    unsigned int *pdev_led_hwinfo;
    /* 定义数据寄存器的虚拟地址变量*/
    struct resource *mem_dr;
    /* 定义数据方向寄存器的虚拟地址变量 */
    struct resource *mem_ddr;
    /* 定义返回值变量 */
    int ret = 0;
    /* 定义主设备号变量 */
    int major;
    /* 定义次设备号变量 */
    int minor;

    /* 用于存储从 DTS 中读取的 32 位无符号整数数组。 */
    unsigned int gpio_base_addr[1];
    /* 计算得到的 GPIO_DR 地址*/
    u32 gpio_dr_addr;
    /* 计算得到的 GPIO_DDR 地址*/
    u32 gpio_ddr_addr;

    /* 定义临时变量，用于存储寄存器的值 */
    unsigned int val = 0;

    /* 打印平台驱动探测信息 */
    printk("led platform driver probe\n");

    /* 第一步：提取平台设备提供的资源 */
    /* 使用 devm_kzalloc 函数为 led_chrdev 结构体分配内存并清零 */
    led_cdev = devm_kzalloc(&pdev->dev, sizeof(struct led_chrdev), GFP_KERNEL);
    if (!led_cdev)
        return -ENOMEM;

    if (pdev->dev.of_node) {
        /* 设备树匹配 */
        /* 获取led的设备树节点 */
        led_device_node = of_find_node_by_path("/led_test");
        if (led_device_node == NULL) {
            printk("get led_test failed!\n");
            return -ENOMEM;
        }

        /* 获取led_test节点的子节点 */
        led_cdev->device_node = of_find_node_by_name(led_device_node, "led");
        if (led_cdev->device_node == NULL) {
            printk("get led device node failed!\n");
            return -ENOMEM;
        }

        /* 获取 GPIO_BASE 地址 */
        ret = of_property_read_u32_array(led_cdev->device_node,"reg",gpio_base_addr, 1);
        if(ret != 0)
        {
            /* 如果读取失败，打印错误信息并返回 -1 */
            printk("get gpio_base_addr failed!\n");
            return -ENOMEM;
        }

        /* 打印 GPIO_BASE 地址 */
        printk("GPIO_BASE address: 0x%08X\n", gpio_base_addr[0]);

        /* 从设备树获取引脚是高位或低位 */
        ret = of_property_read_u32(led_cdev->device_node, "high-low-position", &led_cdev->hl_pos);
        if(ret < 0) 
        {
            printk(KERN_ERR "Failed to get high-low-position from device tree\n");
            return -EINVAL;
        }

        /* 判断引脚是高位还是低位 */
        if (led_cdev->hl_pos == 0) {
            /* 计算 GPIO_DR 地址并映射，此处是GPIO_DR_L */
            gpio_dr_addr = gpio_base_addr[0] + 0x0000;
        } else {
            /* 计算 GPIO_DR 地址并映射，此处是GPIO_DR_H */
            gpio_dr_addr = gpio_base_addr[0] + 0x0004;
        }

        led_cdev->va_dr = devm_ioremap(&pdev->dev, gpio_dr_addr, 4);
        if (!led_cdev->va_dr) {
            printk(KERN_ERR "Failed to ioremap GPIO_DR\n");
            return -ENOMEM;
        }

        /* 判断引脚是高位还是低位 */
        if (led_cdev->hl_pos == 0) {
            /* 计算 GPIO_DDR 地址并映射，此处是GPIO_DDR_L */
            gpio_ddr_addr = gpio_base_addr[0] + 0x0008;
        } else {
            /* 计算 GPIO_DDR 地址并映射，此处是GPIO_DDR_H */
            gpio_ddr_addr = gpio_base_addr[0] + 0x000C;
        }

        led_cdev->va_ddr = devm_ioremap(&pdev->dev, gpio_ddr_addr, 4);
        if (!led_cdev->va_ddr) {
            printk(KERN_ERR "Failed to ioremap GPIO_DDR\n");
            return -ENOMEM;
        }

        /* 从设备树获取引脚偏移量 */
        if (of_property_read_u32(led_cdev->device_node, "led-pin", &led_cdev->led_pin) < 0) {
            printk(KERN_ERR "Failed to get led pin offset from device tree\n");
            return -EINVAL;
        }
    } else {
        /* 平台设备匹配 */
        /* 获取平台设备的私有数据，得到 LED 灯的引脚偏移量 */
        pdev_led_hwinfo = dev_get_platdata(&pdev->dev);
        if (!pdev_led_hwinfo) {
            return -ENOMEM;
        }
        led_cdev->led_pin = pdev_led_hwinfo[0];

        /* 获取平台设备的资源，得到 GPIO 数据寄存器和数据方向寄存器的资源结构体 */
        mem_dr = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        mem_ddr = platform_get_resource(pdev, IORESOURCE_MEM, 1);

        /* 使用 devm_ioremap 将物理地址映射为虚拟地址，注意该函数分配的内存会自动释放，不需要手动调用iounmap */
        led_cdev->va_dr = devm_ioremap(&pdev->dev, mem_dr->start, resource_size(mem_dr));
        led_cdev->va_ddr = devm_ioremap(&pdev->dev, mem_ddr->start, resource_size(mem_ddr));
        if (!led_cdev->va_dr || !led_cdev->va_ddr) {
            /* 打印映射失败信息 */
            printk("fail to ioremap GPIO registers\n");
            ret = -ENOMEM;
            /* 跳转到错误处理标签 */
            goto ioremap_err;
        }
    }

    /* 设置输出模式 */
    /* 读取数据方向寄存器的值 */
    val = ioread32(led_cdev->va_ddr);
    /* 设置高 16 位的使能位 */
    val |= ((unsigned int)0x1 << (led_cdev->led_pin + 16));
    /* 设置低 16 位的对应引脚为输出模式 */
    val |= ((unsigned int)0x1 << (led_cdev->led_pin));
    /* 将修改后的值写回到数据方向寄存器 */
    iowrite32(val, led_cdev->va_ddr);

    /* 输出高电平 */
    /* 读取数据寄存器的值 */
    val = ioread32(led_cdev->va_dr);
    /* 设置高 16 位的使能位 */
    val |= ((unsigned int)0x1 << (led_cdev->led_pin + 16));
    /* 设置低 16 位的对应引脚输出高电平 */
    val |= ((unsigned int)0x1 << (led_cdev->led_pin));
    /* 将修改后的值写回到数据寄存器 */
    iowrite32(val, led_cdev->va_dr);

    /* 第二步：初始化注册字符设备 */
    /* 分配设备号 */
    ret = alloc_chrdev_region(&devno, 0, DEV_CNT, DEV_NAME);
    if (ret < 0) {
        /* 打印设备号分配失败信息 */
        printk("fail to alloc devno\n");
        /* 跳转到错误处理标签 */
        goto ioremap_err;
    }
    /* 获取主设备号 */
    major = MAJOR(devno);
    /* 获取次设备号 */
    minor = MINOR(devno);
    /* 打印主设备号和次设备号 */
    printk("major=%d, minor=%d\n", major, minor);

    /* 初始化字符设备 */
    cdev_init(&led_cdev->dev, &pdrv_led_fops);
    led_cdev->dev.owner = THIS_MODULE;

    /* 添加字符设备 */
    ret = cdev_add(&led_cdev->dev, devno, DEV_CNT);
    if (ret < 0) {
        /* 打印字符设备添加失败信息 */
        printk("fail to add cdev\n");
        /* 跳转到错误处理标签 */
        goto add_err;
    }

    /* 创建设备类 */
    class = class_create(THIS_MODULE, DEV_NAME);
    if (IS_ERR(class)) {
        /* 打印设备类创建失败信息 */
        printk("fail to create class\n");
        ret = PTR_ERR(class);
        /* 跳转到错误处理标签 */
        goto class_err;
    }

    /* 创建设备节点 */
    device = device_create(class, NULL, devno, NULL, DEV_NAME);
    if (IS_ERR(device)) {
        /* 打印设备节点创建失败信息 */
        printk("fail to create device\n");
        ret = PTR_ERR(device);
        /* 跳转到错误处理标签 */
        goto device_err;
    }

    /* 保存驱动数据 */
    platform_set_drvdata(pdev, led_cdev);

    return 0;

device_err:
    /* 销毁设备类 */
    class_destroy(class);

class_err:
    /* 删除字符设备 */
    cdev_del(&led_cdev->dev);

add_err:
    /* 释放设备号 */
    unregister_chrdev_region(devno, DEV_CNT);

ioremap_err:
    return ret;
}

/* 
 * pdrv_led_remove 函数是平台驱动的移除函数。
 * 功能：释放字符设备相关资源，包括设备节点、设备类、字符设备和设备号等。
 */
static int pdrv_led_remove(struct platform_device *pdev)
{
    struct led_chrdev *led_cdev = platform_get_drvdata(pdev);

    /* 打印平台驱动移除信息 */
    printk("pdrv_led remove\n");

    /* 销毁设备节点 */
    device_destroy(class, devno);

    /* 删除字符设备 */
    cdev_del(&led_cdev->dev);

    /* 释放设备号 */
    unregister_chrdev_region(devno, DEV_CNT);

    /* 销毁设备类 */
    class_destroy(class);

    return 0;
}

/* 定义平台设备 ID 表，用于平台总线的匹配 */
static struct platform_device_id pdev_led_ids[] = {
    {.name = "pdev_led"},
    {}
};

/* 声明平台设备 ID 表，供内核使用 */
MODULE_DEVICE_TABLE(platform, pdev_led_ids);

static const struct of_device_id dts_led_of_match[] = {
    { .compatible = "fire,led_test" },
    {}
};

MODULE_DEVICE_TABLE(of, dts_led_of_match);

/* 定义平台驱动结构体，包含探测、移除函数，驱动名称和设备 ID 表 */
static struct platform_driver pdrv_led = {
    .probe = pdrv_led_probe,
    .remove = pdrv_led_remove,
    .id_table = pdev_led_ids,
    .driver     = {
        .name    = "dts_led",
        .of_match_table = dts_led_of_match,
    },
};

/* 
 * pdrv_led_init 函数是模块的初始化函数。
 * 功能：注册平台驱动。
 */
static __init int pdrv_led_init(void)
{
    /* 打印平台驱动初始化信息 */
    printk("led platform driver init\n");
    /* 注册平台驱动 */
    platform_driver_register(&pdrv_led);

    return 0;
}

/* 
 * pdrv_led_exit 函数是模块的退出函数。
 * 功能：注销平台驱动。
 */
static __exit void pdrv_led_exit(void)
{
    /* 打印平台驱动退出信息 */
    printk("led platform driver exit\n");
    /* 注销平台驱动 */
    platform_driver_unregister(&pdrv_led);
}

module_init(pdrv_led_init);
module_exit(pdrv_led_exit);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("dts_led module");
MODULE_LICENSE("GPL");