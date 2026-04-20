#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/sysfs.h>
#include <linux/kernel.h>
#include <linux/mutex.h>

/* 类名称 */
#define CLASS_NAME     "pwm-sysfs"

/* 设备名称 */
#define DEV_NAME       "pwm-test"

/* PWM控制参数结构体 */
struct pwm_param {
    unsigned int period;      /* PWM周期，单位：ns */
    unsigned int duty_cycle;  /* PWM占空比，单位：ns */
    unsigned int polarity;    /* PWM极性：0-正常 1-反相 */
    unsigned int enable;      /* PWM使能：0-关闭 1-开启 */
};

/* PWM私有设备结构体 */
struct pwm_dev {
    struct class *class;       /* 设备类 */
    struct device *device;     /* 设备节点 */
    struct pwm_device *pwm;    /* PWM设备句柄 */
    struct pwm_param param;    /* PWM控制参数 */
    struct mutex lock;         /* 互斥锁 */
};

/* enable节点show函数：读取使能状态 */
static ssize_t enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{   
    ssize_t ret;
    /* 获取私有数据 */
    struct pwm_dev *pwm_priv = dev_get_drvdata(dev); 

    /* 上锁 */
    mutex_lock(&pwm_priv->lock);

    /* 数字转字符串，让用户能看懂内核的参数 */
    ret = sprintf(buf, "%u\n", pwm_priv->param.enable);

    /* 解锁 */
    mutex_unlock(&pwm_priv->lock); 

    return ret;
}

/* enable节点store函数：设置使能/关闭 */
static ssize_t enable_store(struct device *dev, struct device_attribute *attr,
                            const char *buf, size_t count)
{
    int ret;
    unsigned int val;
    /* 获取私有数据 */
    struct pwm_dev *pwm_priv = dev_get_drvdata(dev); 

    /* 上锁 */
    mutex_lock(&pwm_priv->lock);

    /* 按10进制转换，把用户写入的字符串转换成内核能用的数字 */
    ret = kstrtouint(buf, 10, &val);
    if (ret || (val != 0 && val != 1)) {
        dev_err(dev, "enable must be 0 or 1\n");
        mutex_unlock(&pwm_priv->lock);
        return -EINVAL;
    }

    pwm_priv->param.enable = val;

    /* 配置PWM使能/关闭 */
    if (pwm_priv->param.enable)
        pwm_enable(pwm_priv->pwm);
    else
        pwm_disable(pwm_priv->pwm);

    /* 解锁 */
    mutex_unlock(&pwm_priv->lock); 

    return count;
}

/* period节点show函数：读取周期 */
static ssize_t period_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret;
    /* 获取私有数据 */
    struct pwm_dev *pwm_priv = dev_get_drvdata(dev); 

    /* 上锁 */
    mutex_lock(&pwm_priv->lock);

    /* 数字转字符串，让用户能看懂内核的参数 */
    ret = sprintf(buf, "%u\n", pwm_priv->param.period);

    /* 解锁 */
    mutex_unlock(&pwm_priv->lock); 

    return ret;
}

/* period节点store函数：设置周期 */
static ssize_t period_store(struct device *dev, struct device_attribute *attr,
                            const char *buf, size_t count)
{
    int ret;
    unsigned int val;
    /* 获取私有数据 */
    struct pwm_dev *pwm_priv = dev_get_drvdata(dev); 

    /* 上锁 */
    mutex_lock(&pwm_priv->lock);

    /* 按10进制转换，把用户写入的字符串转换成内核能用的数字 */
    ret = kstrtouint(buf, 10, &val);
    if (ret || val == 0) {
        dev_err(dev, "invalid period value\n");
        mutex_unlock(&pwm_priv->lock);
        return -EINVAL;
    }

    pwm_priv->param.period = val;

    /* 重新配置PWM */
    ret = pwm_config(pwm_priv->pwm, pwm_priv->param.duty_cycle, pwm_priv->param.period);

    /* 解锁 */
    mutex_unlock(&pwm_priv->lock); 

    return ret ? ret : count;
}

/* duty_cycle节点show函数：读取占空比 */
static ssize_t duty_cycle_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret;
    /* 获取私有数据 */
    struct pwm_dev *pwm_priv = dev_get_drvdata(dev); 

    /* 上锁 */
    mutex_lock(&pwm_priv->lock);

    /* 数字转字符串，让用户能看懂内核的参数 */
    ret = sprintf(buf, "%u\n", pwm_priv->param.duty_cycle);

    /* 解锁 */
    mutex_unlock(&pwm_priv->lock); 

    return ret;
}

/* duty_cycle节点store函数：设置占空比 */
static ssize_t duty_cycle_store(struct device *dev, struct device_attribute *attr,
                                const char *buf, size_t count)
{
    int ret;
    unsigned int val;
    /* 获取私有数据 */
    struct pwm_dev *pwm_priv = dev_get_drvdata(dev); 

    /* 上锁 */
    mutex_lock(&pwm_priv->lock);

    /* 按10进制转换，把用户写入的字符串转换成内核能用的数字 */
    ret = kstrtouint(buf, 10, &val);
    if (ret || val > pwm_priv->param.period) {
        dev_err(dev, "invalid duty_cycle value\n");
        mutex_unlock(&pwm_priv->lock);
        return -EINVAL;
    }

    pwm_priv->param.duty_cycle = val;

    /* 重新配置PWM */
    ret = pwm_config(pwm_priv->pwm, pwm_priv->param.duty_cycle, pwm_priv->param.period);

    /* 解锁 */
    mutex_unlock(&pwm_priv->lock); 

    return ret ? ret : count;
}

/* polarity节点show函数：读取极性 */
static ssize_t polarity_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret;
    /* 获取私有数据 */
    struct pwm_dev *pwm_priv = dev_get_drvdata(dev); 

    /* 上锁 */
    mutex_lock(&pwm_priv->lock);

    /* 数字转字符串，让用户能看懂内核的参数 */
    ret = sprintf(buf, "%u\n", pwm_priv->param.polarity);

    /* 解锁 */
    mutex_unlock(&pwm_priv->lock); 

    return ret;
}

/* polarity节点store函数：设置极性 */
static ssize_t polarity_store(struct device *dev, struct device_attribute *attr,
                            const char *buf, size_t count)
{
    int ret;
    unsigned int val;
    /* 获取私有数据 */
    struct pwm_dev *pwm_priv = dev_get_drvdata(dev); 

    /* 上锁 */
    mutex_lock(&pwm_priv->lock);

    /* 按10进制转换，把用户写入的字符串转换成内核能用的数字 */
    ret = kstrtouint(buf, 10, &val);
    if (ret || (val != 0 && val != 1)) {
        dev_err(dev, "polarity must be 0 or 1\n");
        mutex_unlock(&pwm_priv->lock);
        return -EINVAL;
    }

    pwm_priv->param.polarity = val;

    /* 配置PWM极性 */
    ret = pwm_set_polarity(pwm_priv->pwm, pwm_priv->param.polarity);
    
    /* 解锁 */
    mutex_unlock(&pwm_priv->lock); 

    return ret ? ret : count;
}

/* 定义sysfs设备属性 */
static DEVICE_ATTR_RW(enable);
static DEVICE_ATTR_RW(period);
static DEVICE_ATTR_RW(duty_cycle);
static DEVICE_ATTR_RW(polarity);

/* 注册属性到属性组 */
static struct attribute *pwm_sysfs_attrs[] = {
    &dev_attr_enable.attr,
    &dev_attr_period.attr,
    &dev_attr_duty_cycle.attr,
    &dev_attr_polarity.attr,
    NULL,
};

/* 定义属性组 */
static struct attribute_group pwm_sysfs_attr_group = {
    .attrs = pwm_sysfs_attrs,
};

/* PWM驱动 probe 实现 */
static int pwm_probe(struct platform_device *pdev)
{
    /* 定义返回值变量 */
    int ret = 0;

    /* 定义PWM私有设备结构体指针 */
    struct pwm_dev *pwm_priv;

    printk(KERN_INFO "pwm driver probe\n");

    /* 分配 PWM 私有设备内存 */
	pwm_priv = devm_kzalloc(&pdev->dev, sizeof(*pwm_priv), GFP_KERNEL);
	if (!pwm_priv)
		return -ENOMEM;

    /* 初始化互斥锁 */
    mutex_init(&pwm_priv->lock);

    /* 从设备树获取 PWM 硬件资源 */
    pwm_priv->pwm = devm_pwm_get(&pdev->dev, NULL);
    if (IS_ERR(pwm_priv->pwm)) {
        ret = PTR_ERR(pwm_priv->pwm);
        printk(KERN_ERR "pwm get resource failed\n");
        return ret;
    }

    /* PWM 参数初始化 */
    pwm_priv->param.period = 10000;
    pwm_priv->param.duty_cycle = 5000;
    pwm_priv->param.polarity = PWM_POLARITY_NORMAL;
    pwm_priv->param.enable = 0;

    /* 配置默认周期占空比 */
    ret = pwm_config(pwm_priv->pwm, pwm_priv->param.duty_cycle, pwm_priv->param.period);
    if (ret) {
        printk(KERN_ERR "pwm default config failed\n");
        return ret;
    }

    /* 配置默认极性 */
    ret = pwm_set_polarity(pwm_priv->pwm, pwm_priv->param.polarity);
    if (ret) {
        printk(KERN_ERR "pwm default polarity failed\n");
        return ret;
    }

    /* 创建设备类 */
    pwm_priv->class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(pwm_priv->class)) {
        ret = PTR_ERR(pwm_priv->class);
        printk(KERN_ERR "pwm class create failed\n");
        return ret;
    }

    /* 创建设备节点 */
    pwm_priv->device = device_create(pwm_priv->class, &pdev->dev,
                                    MKDEV(0, 0), NULL, DEV_NAME);
    if (IS_ERR(pwm_priv->device)) {
        ret = PTR_ERR(pwm_priv->device);
        printk(KERN_ERR "pwm device create failed\n");
        goto destroy_class;
    }

    /* 绑定私有数据到设备 */
    dev_set_drvdata(pwm_priv->device, pwm_priv);
    /* 绑定私有数据到平台设备 */
    dev_set_drvdata(&pdev->dev, pwm_priv);

    /* 创建sysfs接口 */
    ret = sysfs_create_group(&pwm_priv->device->kobj, &pwm_sysfs_attr_group);
    if (ret) {
        printk(KERN_ERR "sysfs create group failed\n");
        goto destroy_device;
    }

    return 0;

destroy_device:
    /* 销毁设备节点 */
    device_destroy(pwm_priv->class, MKDEV(0, 0));

destroy_class:
    /* 销毁设备类 */
    class_destroy(pwm_priv->class);
    return ret;
}

/* PWM驱动 remove 实现 */
static int pwm_remove(struct platform_device *pdev)
{
    /* 获取私有数据 */
    struct pwm_dev *pwm_priv = dev_get_drvdata(&pdev->dev);

    /* 关闭PWM输出 */
    pwm_disable(pwm_priv->pwm);

    /* 删除属性组*/
    sysfs_remove_group(&pwm_priv->device->kobj, &pwm_sysfs_attr_group);

    /* 销毁设备节点 */
    device_destroy(pwm_priv->class, MKDEV(0, 0));

    /* 销毁设备类 */
    class_destroy(pwm_priv->class);

    /* 清空PWM设备句柄 */
    pwm_priv->pwm = NULL;

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
        .name  = "pwm_sysfs",
        .owner = THIS_MODULE,
        .of_match_table = pwm_of_match,
    },
};

module_platform_driver(pwm_driver);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("pwm_sysfs module");
MODULE_LICENSE("GPL");