#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>

/* 声明外部的总线类型结构体 xbus，表明该结构体在其他文件中定义 */
extern struct bus_type xbus;

/*
 * 设备释放函数，当设备被销毁时调用
 * 打印当前文件和函数名，方便调试和跟踪设备释放过程
 * 参数：dev：指向要释放的设备结构体的指针
 */
void xdev_release(struct device *dev)
{
    /* 打印当前文件和函数名 */
    printk("%s-%s\n", __FILE__, __func__);
}

/* 定义一个无符号长整型变量 id，用于存储设备的 ID */
unsigned long id = 0;

/*
 * show 回调函数，用于显示设备属性值
 * 在该函数中，直接将 id 的值通过 sprintf 函数拷贝至 buf 中
 * 用户可以通过 cat 命令查看该属性值
 * 参数：
 * dev：指向设备结构体的指针
 * attr：指向设备属性结构体的指针
 * buf：用于存储属性值的缓冲区
 * 返回值：
 * 实际写入缓冲区的字符数
 */
ssize_t xdev_id_show(struct device *dev, struct device_attribute *attr,
                     char *buf)
{
    /* 将 id 的值以十进制形式写入缓冲区，并返回写入的字符数 */
    return sprintf(buf, "%ld\n", id);
}

/*
 * store 回调函数，用于存储用户输入的设备属性值
 * 利用 kstrtoul 函数将 buf 中的内容转换为 10 进制的数传递给 id
 * 实现了通过 sysfs 修改驱动中 id 值的目的
 * 参数：
 * dev：指向设备结构体的指针
 * attr：指向设备属性结构体的指针
 * buf：包含用户输入值的缓冲区
 * count：缓冲区中字符的数量
 * 返回值：
 * 处理的字符数
 */
ssize_t xdev_id_store(struct device * dev, struct device_attribute * attr,
                      const char *buf, size_t count)
{
    int val = 0;
    /* 将 buf 中的字符串转换为无符号长整型数，并存储到 id 中 */
    val = kstrtoul(buf, 10, &id);
    /* 返回处理的字符数 */
    return count;
}

/*
 * 使用 DEVICE_ATTR 宏定义设备属性 xdev_id
 * 设置该文件的文件权限是文件拥有者可读可写，组内成员以及其他成员不可操作
 * 并指定了 show 和 store 回调函数
 */
DEVICE_ATTR(xdev_id, S_IRUSR | S_IWUSR, xdev_id_show, xdev_id_store);

/*
 * 定义一个设备结构体 xdev
 * init_name：设备的初始名称
 * bus：指向设备所属总线的指针
 * release：设备释放函数的指针
 */
static struct device xdev = {
    .init_name = "xdev",
    .bus = &xbus,
    .release = xdev_release,
};

/*
 * 设备初始化函数，在模块加载时调用
 * 负责注册设备结构体以及创建设备属性文件
 * 返回值：
 * 0：注册成功
 * 非 0：注册失败
 */
static __init int xdev_init(void)
{
    int ret;
    /* 打印设备初始化信息 */
    printk("xdev init\n");
    /* 注册设备结构体 */
    ret = device_register(&xdev);
    if (ret)
        /* 若注册失败，打印错误信息 */
        printk("Unable to register xdev\n");
    /* 创建设备属性文件 */
    device_create_file(&xdev, &dev_attr_xdev_id);
    return 0;
}

/*
 * 设备退出函数，在模块卸载时调用
 * 负责移除设备属性文件并注销设备结构体
 */
static __exit void xdev_exit(void)
{
    /* 打印设备退出信息 */
    printk("xdev exit\n");
    /* 移除设备属性文件 */
    device_remove_file(&xdev, &dev_attr_xdev_id);
    /* 注销设备结构体 */
    device_unregister(&xdev);
}

module_init(xdev_init);
module_exit(xdev_exit);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("xdev module");
MODULE_LICENSE("GPL");