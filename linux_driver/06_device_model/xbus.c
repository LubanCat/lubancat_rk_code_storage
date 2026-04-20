#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>

/* 自定义BUS_ATTR宏，用于定义总线属性，解决高版本内核编译驱动模块报错 */
#define BUS_ATTR(_name, _mode, _show, _store)       \
           struct bus_attribute bus_attr_##_name = __ATTR(_name, _mode, _show, _store)

/*
 * 函数负责总线下的设备以及驱动匹配
 * 使用字符串比较的方式，通过对比驱动以及设备的名字来确定是否匹配
 * 如果相同，则说明匹配成功，返回1；反之，则返回0
 */
int xbus_match(struct device *dev, struct device_driver *drv)
{
    /* 打印当前文件和函数名，方便调试 */
    printk("%s-%s\n", __FILE__, __func__);
    /* 比较设备名和驱动名，前strlen(drv->name)个字符相同则匹配成功 */
    if (!strncmp(dev_name(dev), drv->name, strlen(drv->name))) {
        /* 打印匹配成功信息 */
        printk("dev & drv match\n");
        return 1;
    }
    return 0;
}

/* 定义存放总线名字的变量 */
static char *bus_name = "xbus";

/* 提供show回调函数，使用户可通过cat命令查询总线名称 */
ssize_t xbus_test_show(struct bus_type *bus, char *buf)
{
    /* 将总线名称写入缓冲区并返回写入字符数 */
    return sprintf(buf, "%s\n", bus_name);
}

/* 设置文件权限为文件拥有者可读，组内及其他成员不可操作 */
BUS_ATTR(xbus_test, S_IRUSR, xbus_test_show, NULL);

/*
 * 定义了一种新的总线，名为xbus
 * 总线结构体中最重要的一个成员，便是match回调函数
 */
static struct bus_type xbus = {
    /* 总线的名称 */
    .name = "xbus",
    /* 总线的匹配回调函数，用于设备和驱动的匹配 */
    .match = xbus_match,
};

/* 导出xbus符号，供其他模块使用该总线 */
EXPORT_SYMBOL(xbus);

/* 注册总线，模块初始化时调用 */
static __init int xbus_init(void)
{
    int ret;
    /* 打印总线初始化信息 */
    printk("xbus init\n");
    /* 注册xbus总线 */
    ret = bus_register(&xbus);
    if (ret)
        /* 若注册失败，打印失败信息 */
        printk("xbus register failed\n");
    /* 创建总线属性文件 */
    ret = bus_create_file(&xbus, &bus_attr_xbus_test);
    return 0;
}

/* 注销总线，模块退出时调用 */
static __exit void xbus_exit(void)
{
    /* 打印总线退出信息 */
    printk("xbus exit\n");
    /* 移除总线属性文件 */
    bus_remove_file(&xbus, &bus_attr_xbus_test);
    /* 注销xbus总线 */
    bus_unregister(&xbus);
}

module_init(xbus_init);
module_exit(xbus_exit);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("xbus module");
MODULE_LICENSE("GPL");