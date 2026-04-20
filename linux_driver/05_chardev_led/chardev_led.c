/*
 * 为了方便实验整理得RK各芯片各GPIO寄存器基地址
 * RK3528：
 *  GPIO0：0xff210000
 *  GPIO1：0xff220000
 *  GPIO2：0xff230000
 *  GPIO3：0xff240000
 * 
 * RK3562：
 *  GPIO0：0xff260000
 *  GPIO1：0xff620000
 *  GPIO2：0xff630000
 *  GPIO3：0xffac0000
 *  GPIO4：0xffad0000
 * 
 * RK3566/RK3568：
 *  GPIO0：0xfdd60000
 *  GPIO1：0xfe740000
 *  GPIO2：0xfe750000
 *  GPIO3：0xfe760000
 *  GPIO4：0xfe770000
 *
 * RK3588S/RK3588：
 *  GPIO0：0xfd8a0000
 *  GPIO1：0xfec20000
 *  GPIO2：0xfec30000
 *  GPIO3：0xfec40000
 *  GPIO4：0xfec50000
 *
 * 为了方便实验整理得各板卡的系统心跳灯
 * RK3528：
 * LubanCat-Q1、LubanCat-Q1IO：(GPIO4_B5)
 * RK3562：
 * LubanCat-1HS：(GPIO3_A7)
 * RK3566：
 * LubanCat-0、LubanCat-1、LubanCat-1n：(GPIO0_C5)
 * LubanCat-1H：(GPIO0_C7)
 * LubanCat-1IO：(GPIO0_C0)
 * RK3568：
 * LubanCat-2、LubanCat-2-V1、LubanCat-2-V2、LubanCat-2H、LubanCat-2N、LubanCat-2N-V2、LubanCat-2N-V3：(GPIO0_C7)
 * LubanCat-2-V3：(GPIO1_A4)
 * LubanCat-2IO：(GPIO4_D2)
 * RK3576：
 * LubanCat-3、LubanCat-3-V2、LubanCat-3IO：(GPIO3_C5)
 * RK3588S：
 * LubanCat-4、LubanCat-4-V1：(GPIO4_B5)
 * LubanCat-4IO：(GPIO1_C6)
 * RK3588：
 * LubanCat-5、LubanCat-5-V2：(GPIO0_D3)
 * LubanCat-5IO：(GPIO1_C6)
 *
 * 本实验默认以RK3568的GPIO0_C7为例，如果需要修改为其他引脚，需修改
 * 1、GPIO寄存器基地址，RK3568的GPIO0基地址为0xFDD60000
 *    #define GPIO_BASE (0xFDD60000)   
 * 2、引脚偏移，A、B端口的引脚属于低16个引脚，C、D端口的引脚属于高16个引脚，GPIO0_C7中的C7是高16个引脚中索引号为7的引脚。
 *    {.led_pin = 7},
 * 3、数据寄存器和数据方向寄存器映射
 *    A、B端口使用GPIO_DR_L、GPIO_DDR_L
 *    C、D端口使用GPIO_DR_H、GPIO_DDR_H
 *    GPIO0_C7的端口为C，使用GPIO_DR_H、GPIO_DDR_H
 *    ioremap(GPIO_DR_H, 4);
 *    ioremap(GPIO_DDR_H, 4);
 *
 * 如果修改为RK3588S的GPIO4_B5
 *    #define GPIO_BASE (0xFEC50000)               // GPIO4的基地址
 *
 *    static struct led_chrdev led_cdev[DEV_CNT] = {
 *       {.led_pin = 13},                          // 偏移，GPIO4_B5，B端口属于低16个引脚，B5为8+5位，即偏移13
 * 
 *    led_cdev[0].va_dr = ioremap(GPIO_DR_L, 4);   // B端口使用GPIO_DR_L、GPIO_DDR_L
 *    led_cdev[0].va_ddr = ioremap(GPIO_DDR_L, 4);
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>

/* 定义字符设备的名称 */
#define DEV_NAME    "chardev_led"
/* 定义字符设备的数量 */
#define DEV_CNT     1

/* 每组 GPIO 有 2 个寄存器，对应 32 个引脚，每个寄存器负责 16 个引脚；
 * 一个寄存器 32 位，其中高 16 位都是使能位，低 16 位对应 16 个引脚，每个引脚占用 1 比特位
 * 定义 GPIO 寄存器的基地址 */
#define GPIO_BASE (0xFDD60000)
/* 定义 GPIO 数据寄存器低 16 位的地址 */
#define GPIO_DR_L (GPIO_BASE + 0x0000)
/* 定义 GPIO 数据寄存器高 16 位的地址 */
#define GPIO_DR_H (GPIO_BASE + 0x0004)
/* 定义 GPIO 数据方向寄存器低 16 位的地址 */
#define GPIO_DDR_L (GPIO_BASE + 0x0008)
/* 定义 GPIO 数据方向寄存器高 16 位的地址 */
#define GPIO_DDR_H (GPIO_BASE + 0x000C)

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
    /* 引脚偏移量 */
    unsigned int led_pin;
};

/* 定义 LED 字符设备数组 */
static struct led_chrdev led_cdev[DEV_CNT] = {
    {.led_pin = 7}, /* 偏移，高 16 引脚，GPIO0_C7 */
};

/* 字符设备打开函数 */
static int chardev_led_open(struct inode *inode, struct file *filp)
{
    /* 通过 inode 中的 i_cdev 指针找到对应的 led_chrdev 结构体 */
    struct led_chrdev *led_cdev = container_of(inode->i_cdev, struct led_chrdev, dev);
    /* 将 led_chrdev 结构体指针存储到文件的私有数据中 */
    filp->private_data = led_cdev;

    /* 打印设备打开信息 */
    printk("chardev_led open\r\n");

    return 0;
}

/* 字符设备释放函数 */
static int chardev_led_release(struct inode *inode, struct file *filp)
{
    printk("chardev_led release\r\n");
    return 0;
}

/* 字符设备写函数 */
static ssize_t chardev_led_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    /* 定义临时变量，用于存储寄存器的值 */
    unsigned long val = 0;
    /* 定义临时变量，用于存储从用户空间读取的数据 */
    char ret = 0;
    /* 从文件的私有数据中获取 led_chrdev 结构体指针 */
    struct led_chrdev *led_cdev = filp->private_data;

    /* 打印设备写操作信息 */
    printk("chardev_led write \r\n");

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
static ssize_t chardev_led_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    return 0;
}

/* 定义字符设备的文件操作结构体 */
static struct file_operations chardev_led_fops = {
    .owner = THIS_MODULE,
    .open = chardev_led_open,
    .release = chardev_led_release,
    .write = chardev_led_write,
    .read = chardev_led_read,
};

/* 模块初始化函数 */
static int __init chrdev_init(void)
{
    /* 定义返回值变量 */
    int ret = 0;
    /* 定义主设备号变量 */
    int major;
    /* 定义次设备号变量 */
    int minor;
    /* 用于存储寄存器的值 */
    unsigned int val = 0;

    /* 打印模块初始化信息 */
    printk("chrdev init\n");

    /* 映射 GPIO 寄存器 */
    /* 映射 GPIO 数据寄存器高 16 位的物理地址到虚拟地址 */
    led_cdev[0].va_dr = ioremap(GPIO_DR_H, 4);
    /* 映射 GPIO 数据方向寄存器高 16 位的物理地址到虚拟地址 */
    led_cdev[0].va_ddr = ioremap(GPIO_DDR_H, 4);
    if (!led_cdev[0].va_dr || !led_cdev[0].va_ddr) {
        /* 打印映射失败信息 */
        printk("fail to ioremap GPIO registers\n");
        ret = -ENOMEM;
        /* 跳转到错误处理标签 */
        goto ioremap_err;
    }

    /* GPIO初始化 */
    /* 设置输出模式 */
    val = ioread32(led_cdev[0].va_ddr);
    val |= ((unsigned int)0x1 << (led_cdev[0].led_pin + 16));
    val |= ((unsigned int)0x1 << (led_cdev[0].led_pin));
    iowrite32(val, led_cdev[0].va_ddr);

    /* 输出高电平 */
    val = ioread32(led_cdev[0].va_dr);
    val |= ((unsigned int)0x1 << (led_cdev[0].led_pin + 16));
    val |= ((unsigned int)0x1 << (led_cdev[0].led_pin));
    iowrite32(val, led_cdev[0].va_dr);

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
    cdev_init(&led_cdev[0].dev, &chardev_led_fops);
    led_cdev[0].dev.owner = THIS_MODULE;

    /* 添加字符设备 */
    ret = cdev_add(&led_cdev[0].dev, devno, DEV_CNT);
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

    return 0;

device_err:
    /* 销毁设备类 */
    class_destroy(class);

class_err:
    /* 删除字符设备 */
    cdev_del(&led_cdev[0].dev);

add_err:
    /* 释放设备号 */
    unregister_chrdev_region(devno, DEV_CNT);

ioremap_err:
    return ret;
}

/* 模块退出函数 */
static void __exit chrdev_exit(void)
{
    /* 打印模块退出信息 */
    printk("chrdev exit!\r\n");

    /* 销毁设备节点 */
    device_destroy(class, devno);

    /* 销毁设备类 */
    class_destroy(class);

    /* 删除字符设备 */
    cdev_del(&led_cdev[0].dev);

    /* 释放设备号 */
    unregister_chrdev_region(devno, DEV_CNT);

    /* 释放 GPIO 数据寄存器的映射 */
    iounmap(led_cdev[0].va_dr);
    /* 释放 GPIO 数据方向寄存器的映射 */
    iounmap(led_cdev[0].va_ddr);
}

module_init(chrdev_init);
module_exit(chrdev_exit);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("chardev_led module");
MODULE_LICENSE("GPL");