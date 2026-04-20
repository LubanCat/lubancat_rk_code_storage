#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/regmap.h>

/* OLED 硬件参数宏定义 */
#define X_WIDTH         128         /* OLED显示屏列数 */
#define Y_WIDTH         64          /* OLED显示屏行数 */

#define DEV_NAME        "i2c_oled_regmap"  /* 设备名称 */
#define DEV_CNT         1           /* 设备节点数量 */

/* 应用层传递的显示数据结构体 */
typedef struct oled_display_struct {
    u8 x;                      /* 显示起始X坐标 */
    u8 y;                      /* 显示起始Y坐标(页) */
    u32 length;                /* 显示数据长度 */
    u8 display_buffer[];       /* 显示数据缓冲区 */
} oled_display_struct;

/* 
 * OLED 私有设备结构体，统一管理字符设备、I2C设备、设备号等资源 
 * I2C无读写方向引脚，用reg地址区分命令/数据：0x00=命令，0x40=数据
 */
struct oled_dev {
    dev_t devno;               /* 字符设备号 */
    struct cdev cdev;          /* 字符设备结构体 */
    struct class *class;       /* 设备类 */
    struct device *device;     /* 设备节点 */
    struct i2c_client *client; /* I2C客户端句柄 */
    struct regmap *regmap;     /* Regmap句柄 */
};

/* 静态全局OLED设备实例 */
static struct oled_dev oled_dev;

/* OLED 初始化指令序列 */
static const u8 oled_init_cmds[] = {
    0xae, 0x00, 0x10, 0x40,
    0x81, 0xcf, 0xa1, 0xc8,
    0xa6, 0xa8, 0x3f, 0xd3,
    0x00, 0xd5, 0x80, 0xd9,
    0xf1, 0xda, 0x12, 0xdb,
    0x40, 0x20, 0x02, 0x8d,
    0x14, 0xa4, 0xa6, 0xaf
};

/* Regmap配置结构体 */
static const struct regmap_config oled_regmap_config = {
    .reg_bits = 8,              // 寄存器地址位数：8位
    .val_bits = 8,              // 寄存器数据位宽：8位
    .can_multi_write = true,    // 支持多字节批量写入
};

/*
 * 函数功能：OLED 写命令
 * client：I2C设备句柄
 * cmd：要写入的命令
 * 返回值：0-成功，负数-失败
 */
static int oled_write_cmd(struct i2c_client *client, u8 cmd)
{
    int ret;

    /* SSD1306 I2C 写命令：reg=0x00 */
    ret = regmap_write(oled_dev.regmap, 0x00, cmd);
    if (ret) {
        printk(KERN_ERR "i2c_oled write cmd failed\n");
        return ret;
    }

    return 0;
}

/*
 * 函数功能：OLED 批量写命令
 * client：I2C设备句柄
 * cmds：命令缓冲区
 * len：命令长度
 * 返回值：0-成功，负数-失败
 */
static int oled_write_cmds(struct i2c_client *client, const u8 *cmds, u16 len)
{
    int ret;

    /* SSD1306 I2C 批量写命令：reg=0x00 */
    ret = regmap_bulk_write(oled_dev.regmap, 0x00, cmds, len);
    if (ret) {
        printk(KERN_ERR "i2c_oled bulk write cmds failed\n");
        return ret;
    }

    return 0;
}

/*
 * 函数功能：OLED 写单字节数据
 * client：I2C设备句柄
 * data：要写入的数据
 * 返回值：0-成功，负数-失败
 */
static int oled_write_data(struct i2c_client *client, u8 data)
{
    int ret;

    /* SSD1306 I2C 写数据：reg=0x40 */
    ret = regmap_write(oled_dev.regmap, 0x40, data);
    if (ret) {
        printk(KERN_ERR "i2c_oled write data failed\n");
        return ret;
    }
    return 0;
}

/*
 * 函数功能：OLED 批量写数据
 * client：I2C设备句柄
 * buf：数据缓冲区
 * len：数据长度
 * 返回值：0-成功，负数-失败
 */
static int oled_write_datas(struct i2c_client *client, const u8 *buf, u16 len)
{
    int ret;

    /* SSD1306 I2C 批量写数据：reg=0x40 */
    ret = regmap_bulk_write(oled_dev.regmap, 0x40, buf, len);
    if (ret) {
        printk(KERN_ERR "i2c_oled write datas failed\n");
        return ret;
    }

    return 0;
}

/*
 * 函数功能：OLED 全屏填充
 * dat：填充值
 */
static void oled_fill(u8 dat)
{
    u8 y, x;

    /* 遍历8个页 */
    for (y = 0; y < 8; y++) {
        /* 设置页地址 */
        oled_write_cmd(oled_dev.client, 0xb0 + y);
        /* 设置列地址低4位 */
        oled_write_cmd(oled_dev.client, 0x00);
        /* 设置列地址高4位 */
        oled_write_cmd(oled_dev.client, 0x10);
        /* 整行填充数据 */
        for (x = 0; x < X_WIDTH; x++) {
            oled_write_data(oled_dev.client, dat);
        }
    }
}

/*
 * 函数功能：OLED 指定位置显示数据
 * buf：显示数据
 * x：起始X坐标
 * y：起始Y坐标
 * len：数据长度
 * 返回值：写入长度，负数-失败
 */
static int oled_display(u8 *buf, u8 x, u8 y, u16 len)
{
    int ret = 0;
    u16 index = 0;

    do {
        /* 设置显示起始坐标 */
        ret += oled_write_cmd(oled_dev.client, 0xb0 + y);                  /* 设置页 */
        ret += oled_write_cmd(oled_dev.client, ((x & 0xf0) >> 4) | 0x10);  /* 设置地址高4位 */
        ret += oled_write_cmd(oled_dev.client, (x & 0x0f) | 0x00);         /* 设置地址低4位 */

        /* 剩余数据超过当前行宽度，换行显示 */
        if (len > (X_WIDTH - x)) {
            ret += oled_write_datas(oled_dev.client, buf + index, X_WIDTH - x);
            len -= (X_WIDTH - x);
            index += (X_WIDTH - x);
            x = 0;
            y++;
        } else {
            /* 剩余数据不足一行，直接写入 */
            ret += oled_write_datas(oled_dev.client, buf + index, len);
            index += len;
            len = 0;
        }
    } while (len > 0);

    if (ret) {
        printk(KERN_ERR "i2c_oled display failed\n");
        return -EIO;
    }
    return index;
}

/*
 * 函数功能：OLED 硬件初始化
 */
static void oled_hw_init(void)
{
    /* 写入初始化指令序列 */
    oled_write_cmds(oled_dev.client, oled_init_cmds, ARRAY_SIZE(oled_init_cmds));

    /* 清屏，填充0x00 */
    oled_fill(0x00);
}

/* 字符设备 open 实现 */
static int oled_open(struct inode *inode, struct file *filp)
{
    /* 开启 OLED 显示 */
    oled_write_cmd(oled_dev.client, 0xaf);

    printk(KERN_INFO "i2c_oled device open\n");

    return 0;
}

/* 字符设备 write 实现 */
static ssize_t oled_write(struct file *filp, const char __user *buf,
                          size_t cnt, loff_t *off)
{
    oled_display_struct *data;
    int ret;

    /* 申请内存存储应用层数据 */
    data = kzalloc(cnt, GFP_KERNEL);
    if (!data) {
        printk(KERN_ERR "i2c_oled malloc failed\n");
        return -ENOMEM;
    }

    /* 从用户空间拷贝数据 */
    ret = copy_from_user(data, buf, cnt);
    if (ret) {
        printk(KERN_ERR "i2c_oled copy from user failed\n");
        ret = -EFAULT;
        goto free_mem;
    }

    /* 执行显示 */
    ret = oled_display(data->display_buffer, data->x, data->y, data->length);

    return cnt;

free_mem:
    /* 释放申请的内存 */
    kfree(data);
    return ret;
}

/* 字符设备 release 实现 */
static int oled_release(struct inode *inode, struct file *filp)
{
    /* 关闭 OLED 显示 */
    oled_write_cmd(oled_dev.client, 0xae);

    printk(KERN_INFO "i2c_oled device release\n");

    return 0;
}

/* 字符设备操作集 */
static const struct file_operations oled_fops = {
    .owner      = THIS_MODULE,
    .open       = oled_open,
    .write      = oled_write,
    .release    = oled_release,
};

/* I2C驱动 probe 实现 */
static int oled_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{   
    /* 定义返回值变量 */
    int ret;
    /* 定义主设备号变量 */
    int major;
    /* 定义次设备号变量 */
    int minor;

    printk(KERN_INFO "i2c_oled driver probe\n");

    /* 分配设备号 */
    ret = alloc_chrdev_region(&oled_dev.devno, 0, DEV_CNT, DEV_NAME);
    if (ret) {
        printk(KERN_ERR "fail to alloc devno\n");
        return ret;
    }
    /* 获取主设备号 */
    major = MAJOR(oled_dev.devno);
    /* 获取次设备号 */
    minor = MINOR(oled_dev.devno);
    /* 打印主设备号和次设备号 */
    printk(KERN_INFO "major=%d, minor=%d\n", major, minor);

    /* 初始化字符设备 */
    cdev_init(&oled_dev.cdev, &oled_fops);
    oled_dev.cdev.owner = THIS_MODULE;

    /* 添加字符设备 */
    ret = cdev_add(&oled_dev.cdev, oled_dev.devno, DEV_CNT);
    if (ret) {
        printk(KERN_ERR "fail to add cdev\n");
        goto unreg_chrdev;
    }

    /* 创建设备类 */
    oled_dev.class = class_create(THIS_MODULE, DEV_NAME);
    if (IS_ERR(oled_dev.class)) {
        printk(KERN_ERR "fail to create class\n");
        ret = PTR_ERR(oled_dev.class);
        goto del_cdev;
    }

    /* 创建设备节点 */
    oled_dev.device = device_create(oled_dev.class, &client->dev,
                                    oled_dev.devno, NULL, DEV_NAME);
    if (IS_ERR(oled_dev.device)) {
        printk(KERN_ERR "fail to create device\n");
        ret = PTR_ERR(oled_dev.device);
        goto destroy_class;
    }

    /* 标准I2C Regmap初始化 */
    oled_dev.regmap = devm_regmap_init_i2c(client, &oled_regmap_config);
    if (IS_ERR(oled_dev.regmap)) {
        ret = PTR_ERR(oled_dev.regmap);
        printk(KERN_ERR "regmap init failed\n");
        goto destroy_device;
    }

    /* 保存I2C客户端句柄到私有结构体 */
    oled_dev.client = client;

    /* 初始化OLED硬件 */
    oled_hw_init();

    return 0;

destroy_device:
    /* 销毁设备节点 */
    device_destroy(oled_dev.class, oled_dev.devno);

destroy_class:
    /* 销毁设备类 */
    class_destroy(oled_dev.class);

del_cdev:
    /* 删除字符设备 */
    cdev_del(&oled_dev.cdev);

unreg_chrdev:
    /* 释放设备号 */
    unregister_chrdev_region(oled_dev.devno, DEV_CNT);
    return ret;
}

/* I2C驱动 remove 实现 */
static int oled_i2c_remove(struct i2c_client *client)
{
    /* 打印平台驱动移除信息 */
    printk(KERN_INFO "i2c_oled driver remove\n");

    /* 销毁设备节点 */
    device_destroy(oled_dev.class, oled_dev.devno);

    /* 删除字符设备 */
    cdev_del(&oled_dev.cdev);

    /* 释放设备号 */
    unregister_chrdev_region(oled_dev.devno, DEV_CNT);

    /* 销毁设备类 */
    class_destroy(oled_dev.class);

    /* 清空设备句柄 */
    oled_dev.client = NULL;
    oled_dev.regmap = NULL;

    return 0;
}

/* 定义设备树匹配表 */
static const struct of_device_id oled_of_match[] = {
    { .compatible = "fire,i2c_oled" },
    {}
};

/* I2C驱动结构体 */
static struct i2c_driver oled_i2c_driver = {
    .probe      = oled_i2c_probe,
    .remove     = oled_i2c_remove,
    .driver = {
        .name  = "fire,i2c_oled",
        .owner = THIS_MODULE,
        .of_match_table = oled_of_match,
    },
};

/* 驱动初始化函数 */
static int __init oled_driver_init(void)
{
    int ret;

    printk(KERN_INFO "i2c_oled driver init\n");

    /* 注册i2c总线驱动 */
    ret = i2c_add_driver(&oled_i2c_driver);
    if (ret) {
        printk(KERN_ERR "I2C driver register failed\n");
        return ret;
    }

    return 0;
}

/* 驱动注销函数 */
static void __exit oled_driver_exit(void)
{
    /* 注销I2C驱动 */
    i2c_del_driver(&oled_i2c_driver);

    printk(KERN_INFO "i2c_oled driver exit\n");
}

module_init(oled_driver_init);
module_exit(oled_driver_exit);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("i2c_oled_regmap module");
MODULE_LICENSE("GPL");