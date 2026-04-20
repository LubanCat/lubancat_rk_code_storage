#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>

/* 声明外部的总线类型结构体 xbus，表明该结构体在其他文件中定义 */
extern struct bus_type xbus;

/* 定义一个字符指针 name，用于存储驱动的名称 */
char *name = "xdrv";

/*
 * show 回调函数，用于显示驱动属性值
 * 保证该函数的前缀与驱动属性文件一致，这里将 name 的值通过 sprintf 函数拷贝至 buf 中
 * 用户可以通过 cat 命令查看该属性值
 * 参数：
 * drv：指向设备驱动结构体的指针
 * buf：用于存储属性值的缓冲区
 * 返回值：
 * 实际写入缓冲区的字符数
 */
ssize_t drvname_show(struct device_driver *drv, char *buf)
{
    /* 将驱动名称写入缓冲区，并返回写入的字符数 */
    return sprintf(buf, "%s\n", name);
}

/*
 * 使用 DRIVER_ATTR_RO 宏定义一个只读的驱动属性文件 drvname
 * 该属性文件对应的 show 回调函数为 drvname_show
 */
DRIVER_ATTR_RO(drvname);

/*
 * 驱动探测函数，当驱动和设备匹配成功之后会被调用
 * 打印当前文件和函数名，可用于调试和跟踪驱动匹配情况
 * 参数：
 * dev：指向匹配成功的设备结构体的指针
 * 返回值：
 * 0：表示探测成功
 */
int xdrv_probe(struct device *dev)
{
    /* 打印当前文件和函数名 */
    printk("%s-%s\n", __FILE__, __func__);
    return 0;
}

/*
 * 驱动移除函数，当注销驱动时会被调用
 * 通常用于关闭物理设备的某些功能等操作
 * 打印当前文件和函数名，可用于调试和跟踪驱动卸载情况
 * 参数：
 * dev：指向要移除驱动对应的设备结构体的指针
 * 返回值：
 * 0：表示移除成功
 */
int xdrv_remove(struct device *dev)
{
    /* 打印当前文件和函数名 */
    printk("%s-%s\n", __FILE__, __func__);
    return 0;
}

/*
 * 定义一个设备驱动结构体 xdrv
 * 驱动的名字需要和设备的名字相同，否则无法成功匹配
 * 该驱动挂载在已经注册好的总线 xbus 下
 * 当驱动和设备匹配成功后，会执行 probe 函数
 * 当注销驱动时，会执行 remove 函数
 */
static struct device_driver xdrv = {
    /* 驱动的名称，需与设备名一致以实现匹配 */
    .name = "xdev",
    /* 驱动所属的总线，指向已经注册的 xbus 总线 */
    .bus = &xbus,
    /* 驱动探测函数，匹配成功时调用 */
    .probe = xdrv_probe,
    /* 驱动移除函数，注销驱动时调用 */
    .remove = xdrv_remove,
};

/*
 * 驱动初始化函数，在模块加载时调用
 * 负责注册驱动结构体以及创建设备驱动属性文件
 * 返回值：
 * 0：注册成功
 */
static __init int xdrv_init(void)
{
    int ret;
    /* 打印驱动初始化信息 */
    printk("xdrv init\n");
    /* 注册驱动结构体 */
    ret = driver_register(&xdrv);
    /* 创建设备驱动属性文件 */
    ret = driver_create_file(&xdrv, &driver_attr_drvname);
    return 0;
}

/*
 * 驱动退出函数，在模块卸载时调用
 * 负责移除驱动属性文件并注销驱动结构体
 */
static __exit void xdrv_exit(void)
{
    /* 打印驱动退出信息 */
    printk("xdrv exit\n");
    /* 移除驱动属性文件 */
    driver_remove_file(&xdrv, &driver_attr_drvname);
    /* 注销驱动结构体 */
    driver_unregister(&xdrv);
}

module_init(xdrv_init);
module_exit(xdrv_exit);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("xdrv module");
MODULE_LICENSE("GPL");