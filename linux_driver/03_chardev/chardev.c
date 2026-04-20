#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

/* 定义字符设备的名称 */
#define DEV_NAME    "chardev"
/* 定义要分配的字符设备数量 */
#define DEV_CNT     1

/* 定义读写缓冲区的大小为128字节 */
#define BUFF_SIZE   128
/* 定义一个设备的缓冲区，用于数据读写 */
static char vbuf[BUFF_SIZE];
/* 定义要传输给用户空间的数据 */
static char data[] = {"chardev driver"};

/* 定义设备号，用于标识字符设备 */
static dev_t devno;
/* 定义字符设备结构体，用于管理字符设备 */
static struct cdev chr_dev;
/* 定义设备类结构体指针，用于在sysfs中创建设备类 */
struct class *class;
/* 定义设备结构体指针，用于在/dev目录下创建设备文件 */
struct device *device;

/* 定义主设备号 */
int major;
/* 定义次设备号 */
int minor;

/*
 * 字符设备打开函数，当用户打开字符设备文件时调用
 * 将缓冲区地址赋值给文件结构体的private_data成员
 * 参数：inode 是文件对应的 inode 结构体指针，filp 是文件结构体指针
 * 返回值：0 表示打开成功
 */
static int chr_dev_open(struct inode *inode, struct file *filp)
{
    /* 打印字符设备打开信息 */
    printk("chardev open\n");

    /* 关联vbuf缓冲区 */
    filp->private_data = vbuf;

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
 * 从用户空间拷贝数据到对应的缓冲区
 * 参数：filp 是文件结构体指针，buf 是用户空间缓冲区指针，count 是要写入的字节数，ppos 是文件偏移量指针
 * 返回值：0 表示写入成功
 */
static ssize_t chr_dev_write(struct file *filp, const char __user * buf, size_t count, loff_t *ppos)
{
    int ret = 0;
    /* 获取文件结构体中保存的缓冲区地址 */
    char *vbuf = filp->private_data;

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
 * 将默认数据拷贝到对应的缓冲区，再从缓冲区拷贝到用户空间
 * 参数：filp 是文件结构体指针，buf 是用户空间缓冲区指针，count 是要读取的字节数，ppos 是文件偏移量指针
 * 返回值：0 表示读取成功
 */
static ssize_t chr_dev_read(struct file *filp, char __user * buf, size_t count, loff_t *ppos)
{
    int ret = 0;
    /* 获取文件结构体中保存的缓冲区地址 */
    char *vbuf = filp->private_data;

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

    /* 设置字符设备的所有者为当前模块 */
    chr_dev.owner = THIS_MODULE;
    /* 初始化字符设备结构体，关联文件操作结构体 */
    cdev_init(&chr_dev, &chr_dev_fops);

    /* 将字符设备添加到内核中 */
    ret = cdev_add(&chr_dev, devno, DEV_CNT);
    if (ret < 0)
    {
        /* 若添加失败，打印错误信息并跳转到错误处理标签 */
        printk("fail to add cdev\n");
        goto add_err;
    }

    /* 在sysfs中创建设备类 */
    class = class_create(THIS_MODULE, DEV_NAME);
    if (IS_ERR(class)) {
        /* 若创建设备类失败，打印错误信息并跳转到错误处理标签 */
        printk("fail to add class\n");
        goto class_err;
    }

    /* 创建1个设备文件 */
    device = device_create(class, NULL, MKDEV(major, 0), NULL, DEV_NAME);
    if (IS_ERR(device)) {
        /* 若创建设备文件失败，打印错误信息并跳转到错误处理标签 */
        printk("fail to create device\n");
        goto device_err;
    }
    /* 打印设备文件创建成功信息 */
    printk("device created\n");

    return 0;

device_err:
    /* 若创建设备文件失败，销毁已创建的设备文件和设备类 */
    device_destroy(class, MKDEV(major, 0));
    class_destroy(class);

class_err:
    /* 若创建设备类失败，从内核中移除字符设备 */
    cdev_del(&chr_dev);

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
    /* 打印字符设备驱动退出信息 */
    printk("chrdev exit!\n");

    /* 从内核中移除字符设备 */
    cdev_del(&chr_dev);
    /* 注销已分配的设备号 */
    unregister_chrdev_region(devno, DEV_CNT);

    /* 销毁设备文件 */
    device_destroy(class, MKDEV(major, 0));

    /* 销毁设备类 */
    class_destroy(class);
}

module_init(chrdev_init);
module_exit(chrdev_exit);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("chardev module");
MODULE_LICENSE("GPL");