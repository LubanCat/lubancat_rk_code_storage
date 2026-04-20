#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>
#include <linux/spi/spi.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/regmap.h>

/* OLED 硬件参数宏定义 */
#define X_WIDTH         128         /* OLED显示屏列数 */
#define Y_WIDTH         64          /* OLED显示屏行数 */

#define DEV_NAME        "spi_oled_regmap"  /* 设备名称 */
#define DEV_CNT         1           /* 设备节点数量 */

/* 应用层传递的显示数据结构体 */
typedef struct oled_display_struct {
    u8 x;                      /* 显示起始X坐标 */
    u8 y;                      /* 显示起始Y坐标(页) */
    u32 length;                /* 显示数据长度 */
    u8 display_buffer[];       /* 显示数据缓冲区 */
} oled_display_struct;

/* OLED 私有设备结构体，统一管理字符设备、SPI设备、GPIO、设备号等资源 */
struct oled_dev {
    dev_t devno;               /* 字符设备号 */
    struct cdev cdev;          /* 字符设备结构体 */
    struct class *class;       /* 设备类 */
    struct device *device;     /* 设备节点 */
    struct spi_device *spi;    /* SPI设备句柄 */
    struct regmap *regmap;     /* Regmap句柄 */
    int dc_gpio;               /* DC控制引脚编号 */
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

/* 自定义Regmap SPI写入回调，仅发数据，不发地址 */
static int oled_regmap_spi_write(void *context, unsigned int reg, unsigned int val)
{
    struct spi_device *spi = context;
    u8 data = val;
    // 只发1字节纯数据，匹配SSD1306 SPI协议
    return spi_write(spi, &data, 1);
}

/* Regmap配置结构体 */
static const struct regmap_config oled_regmap_config = {
    .val_bits = 8,                      // 寄存器数据位宽：8位
    .can_multi_write = true,            // 支持多字节批量写入
    .reg_write = oled_regmap_spi_write, // 绑定自定义SPI写入
};

/*
 * 函数功能：OLED 写命令
 * spi：SPI设备句柄
 * cmd：要写入的命令
 * 返回值：0-成功，负数-失败
 */
static int oled_write_cmd(struct spi_device *spi, u8 cmd)
{
    int ret;
    /* 设置DC引脚为低电平，表示传输命令 */
    gpio_set_value(oled_dev.dc_gpio, 0);

    /* Regmap单字节写入命令 */
    ret = regmap_write(oled_dev.regmap, 0x00, cmd);
    if (ret) {
        printk(KERN_ERR "spi_oled write cmd failed\n");
        return ret;
    }

    /* 恢复DC引脚为高电平 */
    gpio_set_value(oled_dev.dc_gpio, 1);
    return 0;
}

/*
 * 函数功能：OLED 批量写命令
 * spi：SPI设备句柄
 * cmds：命令缓冲区
 * len：命令长度
 * 返回值：0-成功，负数-失败
 */
static int oled_write_cmds(struct spi_device *spi, const u8 *cmds, u16 len)
{
    int i, ret;

    /* 循环逐字节写入命令 */
    for (i = 0; i < len; i++) {
        ret = oled_write_cmd(spi, cmds[i]);
        if (ret)
            return ret;
    }
    return 0;
}

/*
 * 函数功能：OLED 写单字节数据
 * spi：SPI设备句柄
 * data：要写入的数据
 * 返回值：0-成功，负数-失败
 */
static int oled_write_data(struct spi_device *spi, u8 data)
{
    int ret;

    /* 设置DC引脚为高电平，表示传输数据 */
    gpio_set_value(oled_dev.dc_gpio, 1);

    /* Regmap单字节写入数据 */
    ret = regmap_write(oled_dev.regmap, 0x00, data);
    if (ret) {
        printk(KERN_ERR "spi_oled write data failed\n");
        return ret;
    }
    return 0;
}

/*
 * 函数功能：OLED 批量写数据
 * spi：SPI设备句柄
 * buf：数据缓冲区
 * len：数据长度
 * 返回值：0-成功，负数-失败
 */
static int oled_write_datas(struct spi_device *spi, const u8 *buf, u16 len)
{
    int ret;

    /* 设置DC引脚为高电平，表示传输数据 */
    gpio_set_value(oled_dev.dc_gpio, 1);

    /* Regmap批量写入数据 */
    ret = regmap_bulk_write(oled_dev.regmap, 0x00, buf, len);
    if (ret) {
        printk(KERN_ERR "spi_oled write datas failed\n");
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
        oled_write_cmd(oled_dev.spi, 0xb0 + y);
        /* 设置列地址低4位 */
        oled_write_cmd(oled_dev.spi, 0x00);
        /* 设置列地址高4位 */
        oled_write_cmd(oled_dev.spi, 0x10);
        /* 整行填充数据 */
        for (x = 0; x < X_WIDTH; x++) {
            oled_write_data(oled_dev.spi, dat);
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
        ret += oled_write_cmd(oled_dev.spi, 0xb0 + y);                  /* 设置页 */
        ret += oled_write_cmd(oled_dev.spi, ((x & 0xf0) >> 4) | 0x10);  /* 设置地址高4位 */
        ret += oled_write_cmd(oled_dev.spi, (x & 0x0f) | 0x00);         /* 设置地址低4位 */
        
        /* 剩余数据超过当前行宽度，换行显示 */
        if (len > (X_WIDTH - x)) {
            ret += oled_write_datas(oled_dev.spi, buf + index, X_WIDTH - x);
            len -= (X_WIDTH - x);
            index += (X_WIDTH - x);
            x = 0;
            y++;
        } else {
            /* 剩余数据不足一行，直接写入 */
            ret += oled_write_datas(oled_dev.spi, buf + index, len);
            index += len;
            len = 0;
        }
    } while (len > 0);

    if (ret) {
        printk(KERN_ERR "spi_oled display failed\n");
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
    oled_write_cmds(oled_dev.spi, oled_init_cmds, ARRAY_SIZE(oled_init_cmds));

    /* 清屏，填充0x00 */
    oled_fill(0x00);
}

/* 字符设备 open 实现 */
static int oled_open(struct inode *inode, struct file *filp)
{
    /* 开启 OLED 显示 */
    oled_write_cmd(oled_dev.spi, 0xaf);

    printk(KERN_INFO "spi_oled device open\n");

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
        printk(KERN_ERR "spi_oled malloc failed\n");
        return -ENOMEM;
    }

    /* 从用户空间拷贝数据 */
    ret = copy_from_user(data, buf, cnt);
    if (ret) {
        printk(KERN_ERR "spi_oled copy from user failed\n");
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
    oled_write_cmd(oled_dev.spi, 0xae);

    printk(KERN_INFO "spi_oled device release\n");

    return 0;
}

/* 字符设备操作集 */
static const struct file_operations oled_fops = {
    .owner      = THIS_MODULE,
    .open       = oled_open,
    .write      = oled_write,
    .release    = oled_release,
};

/* SPI驱动 probe 实现 */
static int oled_spi_probe(struct spi_device *spi)
{   
    /* 定义返回值变量 */
    int ret;
    /* 定义主设备号变量 */
    int major;
    /* 定义次设备号变量 */
    int minor;

    /* 获取设备树中对应的节点指针 */
    struct device_node *np = spi->dev.of_node;

    printk(KERN_INFO "spi_oled driver probe\n");

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
    oled_dev.device = device_create(oled_dev.class, &spi->dev,
                                    oled_dev.devno, NULL, DEV_NAME);
    if (IS_ERR(oled_dev.device)) {
        printk(KERN_ERR "fail to create device\n");
        ret = PTR_ERR(oled_dev.device);
        goto destroy_class;
    }

    /* 从设备树获取DC GPIO引脚 */
    oled_dev.dc_gpio = of_get_named_gpio(np, "dc_control_pin", 0);
    if (oled_dev.dc_gpio < 0) {
        printk(KERN_ERR "Get DC GPIO failed\n");
        goto destroy_device;
    }

    /* 申请GPIO引脚 */
    ret = gpio_request(oled_dev.dc_gpio, "oled_dc_gpio");
    if (ret) {
        printk(KERN_ERR "Request DC GPIO failed\n");
        goto destroy_device;
    }

    /* 设置GPIO为输出模式，默认高电平 */
    gpio_direction_output(oled_dev.dc_gpio, 1);

    /* 配置SPI参数 */
    spi->mode = SPI_MODE_0;
    ret = spi_setup(spi);
    if (ret) {
        printk(KERN_ERR "spi setup failed\n");
        goto free_gpio;
    }

    /* 通用Regmap初始化，绑定自定义SPI回调 */
    oled_dev.regmap = devm_regmap_init(&spi->dev, NULL, spi, &oled_regmap_config);
    if (IS_ERR(oled_dev.regmap)) {
        ret = PTR_ERR(oled_dev.regmap);
        printk(KERN_ERR "regmap init failed\n");
        goto free_gpio;
    }

    /* 保存SPI设备句柄到私有结构体 */
    oled_dev.spi = spi;

    /* 打印配置信息 */
    printk(KERN_INFO "SPI max_speed: %dHz\n", spi->max_speed_hz);
    printk(KERN_INFO "SPI mode: 0x%02X\n", spi->mode);
    printk(KERN_INFO "SPI chip_select = %d\n", (int)spi->chip_select);
    printk(KERN_INFO "SPI bits_per_word = %d\n", (int)spi->bits_per_word);

    /* 初始化OLED硬件 */
    oled_hw_init();

    return 0;

free_gpio:
    /* 释放GPIO */
    gpio_free(oled_dev.dc_gpio);

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

/* SPI驱动 remove 实现 */
static int oled_spi_remove(struct spi_device *spi)
{
    /* 打印平台驱动移除信息 */
    printk(KERN_INFO "spi_oled driver remove\n");

    /* 销毁设备节点 */
    device_destroy(oled_dev.class, oled_dev.devno);

    /* 删除字符设备 */
    cdev_del(&oled_dev.cdev);

    /* 释放设备号 */
    unregister_chrdev_region(oled_dev.devno, DEV_CNT);

    /* 销毁设备类 */
    class_destroy(oled_dev.class);

    /* 释放GPIO引脚 */
    gpio_free(oled_dev.dc_gpio);

    /* 清空设备句柄 */
    oled_dev.spi = NULL;
    oled_dev.regmap = NULL;

    return 0;
}

/* 定义设备树匹配表 */
static const struct of_device_id oled_of_match[] = {
    { .compatible = "fire,spi_oled" },
    {}
};

/* SPI驱动结构体 */
static struct spi_driver oled_spi_driver = {
    .probe      = oled_spi_probe,
    .remove     = oled_spi_remove,
    .driver = {
        .name  = "fire,spi_oled",
        .owner = THIS_MODULE,
        .of_match_table = oled_of_match,
    },
};

/* 驱动初始化函数 */
static int __init oled_driver_init(void)
{
    int ret;

    printk(KERN_INFO "spi_oled driver init\n");

    /* 注册SPI总线驱动 */
    ret = spi_register_driver(&oled_spi_driver);
    if (ret) {
        printk(KERN_ERR "SPI driver register failed\n");
        return ret;
    }

    return 0;
}

/* 驱动注销函数 */
static void __exit oled_driver_exit(void)
{
    /* 注销SPI驱动 */
    spi_unregister_driver(&oled_spi_driver);

    printk(KERN_INFO "spi_oled driver exit\n");
}

module_init(oled_driver_init);
module_exit(oled_driver_exit);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("spi_oled_regmap module");
MODULE_LICENSE("GPL");