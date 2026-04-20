#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

/* 定义字符设备的名称 */
#define DEV_NAME    "moredev"
/* 定义要分配的字符设备数量 */
#define DEV_CNT     2

/* 定义读写缓冲区的大小为128字节 */
#define BUFF_SIZE   128

/* 定义要传输给用户空间的数据 */
static char data[] = {"chardev driver"};

/* 定义设备号，用于标识字符设备 */
static dev_t devno;

/*
 * 自定义字符设备结构体
 * 包含一个 cdev 结构体用于字符设备的管理
 * 以及一个大小为 BUFF_SIZE 的缓冲区 vbuf 用于数据读写
 */
struct chr_dev {
    struct cdev dev;
    char vbuf[BUFF_SIZE];
};
/* 定义字符设备结构体数组，用于管理多个字符设备 */
static struct chr_dev chr_devs[DEV_CNT];

/* 定义设备类结构体指针，用于在sysfs中创建设备类 */
struct class *class;
/* 定义设备结构体指针数组，用于在/dev目录下创建设备文件 */
struct device *devices[DEV_CNT];

/* 定义主设备号 */
int major;
/* 定义次设备号 */
int minor;

/*
 * 字符设备打开函数，当用户打开字符设备文件时调用
 * 通过 container_of 宏根据 inode 中的 cdev 找到对应的 chr_dev 结构体
 * 并将其地址赋值给文件结构体的 private_data 成员
 * 参数：inode 是文件对应的 inode 结构体指针，filp 是文件结构体指针
 * 返回值：0 表示打开成功
 */
static int chr_dev_open(struct inode *inode, struct file *filp)
{
    /* 打印字符设备打开信息 */
    printk("chardev open\n");
    /* 通过 container_of 宏获取对应的 chr_dev 结构体地址 */
    filp->private_data = container_of(inode->i_cdev, struct chr_dev, dev);
    return 0;
}

/*
 * 字符设备释放函数，当用户关闭字符设备文件时调用
 * 参数：inode 是文件对应的 inode 结构体指针，filp 是文件结构体指针
 * 返回值：0 表示释放成功
 */
static int chr_dev_release(struct inode *inode, struct file *filp)
{
    /* 打印字符设备释放信息 */
    printk("chardev release\n");
    return 0;
}

/*
 * 字符设备写函数，当用户向字符设备文件写入数据时调用
 * 从用户空间拷贝数据到对应的 chr_dev 结构体的 vbuf 缓冲区
 * 参数：filp 是文件结构体指针，buf 是用户空间缓冲区指针，count 是要写入的字节数，ppos 是文件偏移量指针
 * 返回值：0 表示写入成功
 */
static ssize_t chr_dev_write(struct file *filp, const char __user * buf, size_t count, loff_t *ppos)
{
    int ret = 0;
    /* 获取文件结构体中保存的 chr_dev 结构体指针 */
    struct chr_dev *dev = filp->private_data;
    /* 获取对应的缓冲区地址 */
    char *vbuf = dev->vbuf;
    /* 将用户空间的数据拷贝到内核空间的缓冲区 */
    ret = copy_from_user(vbuf, buf, count);
    if (ret == 0) {
        /* 若拷贝成功，打印写入的数据 */
        printk("write data: %s\n", vbuf);
    } else {
        /* 若拷贝失败，打印写入失败信息 */
        printk("Write failure!\n");
    }
    return 0;
}

/*
 * 字符设备读函数，当用户从字符设备文件读取数据时调用
 * 将默认数据拷贝到对应的 chr_dev 结构体的 vbuf 缓冲区，再从缓冲区拷贝到用户空间
 * 参数：filp 是文件结构体指针，buf 是用户空间缓冲区指针，count 是要读取的字节数，ppos 是文件偏移量指针
 * 返回值：0 表示读取成功
 */
static ssize_t chr_dev_read(struct file *filp, char __user * buf, size_t count, loff_t *ppos)
{
    int ret = 0;
    /* 获取文件结构体中保存的 chr_dev 结构体指针 */
    struct chr_dev *dev = filp->private_data;
    /* 获取对应的缓冲区地址 */
    char *vbuf = dev->vbuf;
    /* 将默认数据拷贝到缓冲区 */
    memcpy(vbuf, data, sizeof(data));
    /* 将缓冲区的数据拷贝到用户空间 */
    ret = copy_to_user(buf, vbuf, count);
    if (ret != 0) {
        /* 若拷贝失败，打印读取失败信息 */
        printk("Read failure!\n");
    }

    return 0;
}

/*
 * 定义字符设备的文件操作结构体，包含了字符设备的各种操作函数
 */
static struct file_operations  chr_dev_fops = 
{
    .owner = THIS_MODULE,
    .open = chr_dev_open,
    .release = chr_dev_release,
    .write = chr_dev_write,
    .read = chr_dev_read,
};

/*
 * 字符设备驱动初始化函数，在模块加载时调用
 * 负责分配设备号、注册字符设备、创建设备类和设备文件
 * 返回值：0 表示初始化成功，非 0 表示初始化失败
 */
static int __init chrdev_init(void)
{
    int ret = 0;
    int i;

    /* 打印字符设备驱动初始化信息 */
    printk("chrdev init\n");

    /* 动态分配字符设备号 */
    ret = alloc_chrdev_region(&devno, 0, DEV_CNT, DEV_NAME);
    if (ret < 0) {
        /* 若分配失败，打印错误信息并跳转到错误处理标签 */
        printk("fail to alloc devno\n");
        goto alloc_err;
    }
    /* 获取主设备号 */
    major = MAJOR(devno);
    /* 获取次设备号 */
    minor = MINOR(devno);
    /* 打印分配到的主设备号和次设备号 */
    printk("major=%d,minor=%d\n", major, minor);
    /* 循环初始化并添加每个字符设备 */
    for (i = 0; i < DEV_CNT; i++) {
        /* 初始化字符设备结构体，关联文件操作结构体 */
        cdev_init(&chr_devs[i].dev, &chr_dev_fops);
        /* 设置字符设备的所有者为当前模块 */
        chr_devs[i].dev.owner = THIS_MODULE;
        /* 将字符设备添加到内核中 */
        ret = cdev_add(&chr_devs[i].dev, MKDEV(major, i), 1);
        if (ret < 0) {
            /* 若添加失败，打印错误信息并跳转到错误处理标签 */
            printk("fail to add cdev for device%d\n", i);
            goto add_err;
        }
    }
    /* 在sysfs中创建设备类 */
    class = class_create(THIS_MODULE, DEV_NAME);
    if (IS_ERR(class)) {
        /* 若创建设备类失败，打印错误信息并跳转到错误处理标签 */
        printk("fail to add class\n");
        goto class_err;
    }

    /* 循环创建设备文件 */
    for (i = 0; i < DEV_CNT; i++) {
        /* 根据主设备号和次设备号创建设备文件 */
        devices[i] = device_create(class, NULL, MKDEV(major, i), NULL, "%s%d", DEV_NAME, i);
        if (IS_ERR(devices[i])) {
            /* 若创建设备文件失败，打印错误信息并跳转到错误处理标签 */
            printk("fail to create device%d\n", i);
            goto device_err;
        }
        /* 打印设备文件创建成功信息 */
        printk("device%d created\n", i);
    }

    return 0;

device_err:
    /* 若创建设备文件失败，销毁已创建的设备文件和设备类 */
    while (i--) {
        device_destroy(class, MKDEV(major, i));
    }
    class_destroy(class);

class_err:
    /* 若创建设备类失败，从内核中移除所有字符设备 */
    for (i = 0; i < DEV_CNT; i++) {
        cdev_del(&chr_devs[i].dev);
    }

add_err:
    /* 若添加字符设备失败，注销已分配的设备号 */
    unregister_chrdev_region(devno, DEV_CNT);

alloc_err:
    return ret;
}

/*
 * 字符设备驱动退出函数，在模块卸载时调用
 * 负责移除字符设备、注销设备号、销毁设备文件和设备类
 */
static void __exit chrdev_exit(void)
{
    int i;

    /* 打印字符设备驱动退出信息 */
    printk("chrdev exit!\n");

    /* 从内核中移除所有字符设备 */
    for (i = 0; i < DEV_CNT; i++) {
        cdev_del(&chr_devs[i].dev);
    }
    /* 注销已分配的设备号 */
    unregister_chrdev_region(devno, DEV_CNT);
    
    /* 循环销毁设备文件 */
    for (i = 0; i < DEV_CNT; i++) {
        device_destroy(class, MKDEV(major, i));
    }

    /* 销毁设备类 */
    class_destroy(class);
}

module_init(chrdev_init);
module_exit(chrdev_exit);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("moredev2 module");
MODULE_LICENSE("GPL");