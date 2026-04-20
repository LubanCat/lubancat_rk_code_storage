#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/uaccess.h>

/* 设备名称 */
#define DEV_NAME        "pwm_subsystem"
/* 设备节点数量 */
#define DEV_CNT         1

/* PWM控制参数结构体 */
typedef struct pwm_config_struct {
    unsigned int period;      /* PWM周期，单位：ns */
    unsigned int duty_cycle;  /* PWM占空比，单位：ns */
    unsigned int polarity;    /* PWM极性：0-正常 1-反相 */
    unsigned int enable;      /* PWM使能：0-关闭 1-开启 */
} pwm_config_struct;

/* PWM私有设备结构体 */
struct pwm_dev {
    dev_t devno;               /* 字符设备号 */
    struct cdev cdev;          /* 字符设备结构体 */
    struct class *class;       /* 设备类 */
    struct device *device;     /* 设备节点 */
    struct pwm_device *pwm;    /* PWM设备句柄 */
};

/* 静态全局PWM设备实例 */
static struct pwm_dev pwm_dev;

/* 字符设备 open 实现 */
static int pwm_open(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "pwm device open\n");

    return 0;
}

/*
 * 函数功能：字符设备 write 实现
 * 应用层write写入pwm_config_struct，内核解析并配置PWM
 */
static ssize_t pwm_write(struct file *filp, const char __user *buf,
                         size_t count, loff_t *off)
{
    int ret = 0;
    /* 定义PWM配置参数结构体 */
    pwm_config_struct *pwm_data;

    /* 校验传入数据长度是否合法 */
    if (count != sizeof(pwm_config_struct)) {
        printk(KERN_ERR "pwm write data length error\n");
        return -EINVAL;
    }

    /* 申请内核内存，存储用户空间数据 */
    pwm_data = kzalloc(count, GFP_KERNEL);
    if (!pwm_data) {
        printk(KERN_ERR "pwm malloc failed\n");
        return -ENOMEM;
    }

    /* 从用户空间拷贝PWM配置数据到内核空间 */
    ret = copy_from_user(pwm_data, buf, count);
    if (ret) {
        printk(KERN_ERR "pwm copy from user failed\n");
        ret = -EFAULT;
        goto free_mem;
    }

    /* 配置PWM周期和占空比 */
    ret = pwm_config(pwm_dev.pwm, pwm_data->duty_cycle, pwm_data->period);
    if (ret) {
        printk(KERN_ERR "pwm config failed\n");
        goto free_mem;
    }

    /* 配置PWM极性 */
    ret = pwm_set_polarity(pwm_dev.pwm, pwm_data->polarity);
    if (ret) {
        printk(KERN_ERR "pwm set polarity failed\n");
        goto free_mem;
    }

    /* 配置PWM使能/关闭 */
    if (pwm_data->enable) {
        pwm_enable(pwm_dev.pwm);
    } else {
        pwm_disable(pwm_dev.pwm);
    }

    /* 打印配置信息 */
    printk(KERN_INFO "pwm set: period=%u ns, duty=%u ns, polarity=%u, enable=%u\n",
           pwm_data->period, pwm_data->duty_cycle,
           pwm_data->polarity, pwm_data->enable);

    /* 释放内核内存 */
    kfree(pwm_data);

    /* 返回成功写入的字节数 */
    return count;

/* 内存释放错误处理 */
free_mem:
    kfree(pwm_data);
    return ret;
}

/* 字符设备 release 实现 */
static int pwm_release(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "pwm device release\n");

    return 0;
}

/* 字符设备操作集 */
static const struct file_operations pwm_fops = {
    .owner      = THIS_MODULE,
    .open       = pwm_open,
    .write      = pwm_write,
    .release    = pwm_release,
};

/* PWM驱动 probe 实现 */
static int pwm_probe(struct platform_device *pdev)
{
    /* 定义返回值变量 */
    int ret = 0;
    /* 定义主设备号变量 */
    int major;
    /* 定义次设备号变量 */
    int minor;

    printk(KERN_INFO "pwm driver probe\n");

    /* 分配设备号 */
    ret = alloc_chrdev_region(&pwm_dev.devno, 0, DEV_CNT, DEV_NAME);
    if (ret) {
        printk(KERN_ERR "pwm alloc chrdev failed\n");
        return ret;
    }

    /* 获取主设备号 */
    major = MAJOR(pwm_dev.devno);
    /* 获取次设备号 */
    minor = MINOR(pwm_dev.devno);
    /* 打印主设备号和次设备号 */
    printk(KERN_INFO "major=%d, minor=%d\n", major, minor);

    /* 初始化字符设备 */
    cdev_init(&pwm_dev.cdev, &pwm_fops);
    pwm_dev.cdev.owner = THIS_MODULE;

    /* 添加字符设备 */
    ret = cdev_add(&pwm_dev.cdev, pwm_dev.devno, DEV_CNT);
    if (ret) {
        printk(KERN_ERR "pwm cdev add failed\n");
        goto unreg_chrdev;
    }

    /* 创建设备类 */
    pwm_dev.class = class_create(THIS_MODULE, DEV_NAME);
    if (IS_ERR(pwm_dev.class)) {
        ret = PTR_ERR(pwm_dev.class);
        printk(KERN_ERR "pwm class create failed\n");
        goto del_cdev;
    }

    /* 创建设备节点 */
    pwm_dev.device = device_create(pwm_dev.class, &pdev->dev,
                                  pwm_dev.devno, NULL, DEV_NAME);
    if (IS_ERR(pwm_dev.device)) {
        ret = PTR_ERR(pwm_dev.device);
        printk(KERN_ERR "pwm device create failed\n");
        goto destroy_class;
    }

    /* 从设备树获取PWM设备资源 */
    pwm_dev.pwm = devm_pwm_get(&pdev->dev, NULL);
    if (IS_ERR(pwm_dev.pwm)) {
        ret = PTR_ERR(pwm_dev.pwm);
        printk(KERN_ERR "pwm get resource failed\n");
        goto destroy_device;
    }

    /* PWM配置默认周期占空比 */
    ret = pwm_config(pwm_dev.pwm, 5000, 10000);
    if (ret) {
        printk(KERN_ERR "pwm default config failed\n");
        goto destroy_device;
    }

    /* PWM配置默认极性 */
    ret = pwm_set_polarity(pwm_dev.pwm, PWM_POLARITY_NORMAL);
    if (ret) {
        printk(KERN_ERR "pwm default polarity failed\n");
        goto destroy_device;
    }

    return 0;

destroy_device:
    /* 销毁设备节点 */
    device_destroy(pwm_dev.class, pwm_dev.devno);

destroy_class:
    /* 销毁设备类 */
    class_destroy(pwm_dev.class);

del_cdev:
    /* 删除字符设备 */
    cdev_del(&pwm_dev.cdev);

unreg_chrdev:
    /* 释放设备号 */
    unregister_chrdev_region(pwm_dev.devno, DEV_CNT);
    return ret;
}

/* PWM驱动 remove 实现 */
static int pwm_remove(struct platform_device *pdev)
{
    /* 关闭PWM输出 */
    pwm_disable(pwm_dev.pwm);

    /* 销毁设备节点 */
    device_destroy(pwm_dev.class, pwm_dev.devno);
    /* 删除字符设备 */
    cdev_del(&pwm_dev.cdev);
    /* 释放字符设备号 */
    unregister_chrdev_region(pwm_dev.devno, DEV_CNT);
    /* 销毁设备类 */
    class_destroy(pwm_dev.class);

    /* 清空PWM设备句柄 */
    pwm_dev.pwm = NULL;

    printk(KERN_INFO "pwm driver remove\n");
    return 0;
}

/* 定义设备树匹配表 */
static const struct of_device_id pwm_of_match[] = {
    { .compatible = "fire,pwm_subsystem" },
    { }
};

/* 导出设备树匹配表 */
MODULE_DEVICE_TABLE(of, pwm_of_match);

/* 平台驱动结构体 */
static struct platform_driver pwm_driver = {
    .probe      = pwm_probe,
    .remove     = pwm_remove,
    .driver = {
        .name  = "pwm_demo",
        .owner = THIS_MODULE,
        .of_match_table = pwm_of_match,
    },
};

/* 驱动初始化函数 */
static int __init pwm_init(void)
{
    return platform_driver_register(&pwm_driver);
}

/* 驱动注销函数 */
static void __exit pwm_exit(void)
{
    platform_driver_unregister(&pwm_driver);
}

module_init(pwm_init);
module_exit(pwm_exit);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("pwm_subsystem module");
MODULE_LICENSE("GPL");