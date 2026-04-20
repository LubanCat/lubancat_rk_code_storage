#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/pm_runtime.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>

/* 休眠状态目标电压：100000uV */
#define VOLT_SUSPEND  100000
/* 唤醒状态目标电压：3300000uV */
#define VOLT_RESUME   3300000

/* 类名称 */
#define CLASS_NAME     "regulator-rpm"

/* sysfs设备名 */
#define DEV_NAME       "regulator-test"

/* 驱动私有数据结构体 */
struct regulator_rpm_dev {
    struct class *class;            /* 设备类 */
    struct device *sysfs_dev;       /* sysfs设备指针 */
    struct device *pm_dev;          /* Runtime PM操作设备指针 */
    struct regulator *regulator;    /* Regulator消费者句柄 */
    struct mutex lock;              /* 互斥锁 */
    int is_on;                      /* 电源状态缓存：0-关闭 1-开启 */
};

/*
 * 函数功能：Runtime PM 挂起回调函数，设备休眠时执行
 * 作用：切换低电压 -> 关闭电源 -> 更新状态缓存
 */
static int reg_pm_suspend(struct device *dev)
{
    /* 获取驱动私有结构体指针 */
    struct regulator_rpm_dev *priv = dev_get_drvdata(dev);
    /* 定义函数返回值变量 */
    int ret;

    /* 加互斥锁 */
    mutex_lock(&priv->lock);

    /* 设置Regulator为休眠电压 */
    ret = regulator_set_voltage(priv->regulator, VOLT_SUSPEND, VOLT_SUSPEND);
    if (ret) {
        printk(KERN_ERR "set suspend voltage failed\n");
        mutex_unlock(&priv->lock);
        return ret;
    }

    /* 关闭Regulator电源输出 */
    ret = regulator_disable(priv->regulator);
    if (ret) {
        printk(KERN_ERR "suspend: disable failed (%d)\n", ret);
        mutex_unlock(&priv->lock);
        return ret;
    }

    /* 更新电源状态缓存为关闭状态 */
    priv->is_on = 0;

    /* 解锁互斥锁*/
    mutex_unlock(&priv->lock);

    /* 打印休眠完成信息 */
    printk(KERN_INFO "PM suspend: regulator OFF\n");

    return 0;
}

/*
 * 函数功能：Runtime PM 唤醒回调函数，设备激活时执行
 * 作用：切换工作电压 -> 打开电源 -> 更新状态缓存
 */
static int reg_pm_resume(struct device *dev)
{
    /* 获取驱动私有结构体指针 */
    struct regulator_rpm_dev *priv = dev_get_drvdata(dev);
    /* 定义函数返回值变量 */
    int ret;

    /* 加互斥锁 */
    mutex_lock(&priv->lock);

    /* 设置Regulator为唤醒电压 */
    ret = regulator_set_voltage(priv->regulator, VOLT_RESUME, VOLT_RESUME);
    if (ret) {
        printk(KERN_ERR "set resume voltage failed\n");
        mutex_unlock(&priv->lock);
        return ret;
    }

    /* 使能Regulator电源输出 */
    ret = regulator_enable(priv->regulator);
    if (ret) {
        printk(KERN_ERR "resume: enable failed (%d)\n", ret);
        mutex_unlock(&priv->lock);
        return ret;
    }

    /* 更新电源状态缓存为开启状态 */
    priv->is_on = 1;

    /* 解锁互斥锁 */
    mutex_unlock(&priv->lock);

    /* 打印唤醒完成信息 */
    printk(KERN_INFO "PM resume: regulator ON\n");

    return 0;
}

/* 注册Runtime PM回调操作集，绑定挂起/唤醒函数 */
static const struct dev_pm_ops reg_pm_ops = {
    /* 宏定义绑定RPM休眠、唤醒、空闲回调，NULL表示无空闲回调 */
    SET_RUNTIME_PM_OPS(reg_pm_suspend, reg_pm_resume, NULL)
};

/*
 * 函数功能：sysfs power_state文件读回调，cat命令执行
 * 作用：读取状态缓存
 */
static ssize_t power_state_show(struct device *dev,
                                struct device_attribute *attr,
                                char *buf)
{
    /* 获取驱动私有结构体指针 */
    struct regulator_rpm_dev *priv = dev_get_drvdata(dev);
    /* 定义状态变量 */
    int state;

    /* 加互斥锁 */
    mutex_lock(&priv->lock);

    /* 从缓存读取电源状态，不操作硬件 */
    state = priv->is_on;

    /* 解锁互斥锁 */
    mutex_unlock(&priv->lock);

    /* 将状态值写入buf，返回给用户态 */
    return sprintf(buf, "%d\n", state);
}

/*
 * 函数功能：sysfs power_state文件写回调，echo命令执行
 * 作用：echo 1触发1s脉冲；echo 0强制立即断电
 */
static ssize_t power_state_store(struct device *dev,
                                 struct device_attribute *attr,
                                 const char *buf,
                                 size_t count)
{
    /* 获取驱动私有结构体指针 */
    struct regulator_rpm_dev *priv = dev_get_drvdata(dev);
    /* 定义用户输入值变量和返回值变量 */
    int val, ret;

    /* 将用户输入字符串转换为整数，校验输入合法性，只能0/1 */
    if (kstrtoint(buf, 10, &val) || val < 0 || val > 1)
        return -EINVAL;

    /* 判断用户输入是否为1，触发脉冲模式 */
    if (val == 1) {

        /* 同步唤醒RPM设备，激活电源 */
        ret = pm_runtime_get_sync(priv->pm_dev);
        if (ret < 0 && ret != -EACCES) {
            pm_runtime_put_noidle(priv->pm_dev);
            return ret;
        }

        /* 释放PM引用，启动自动挂起计时器 */
        pm_runtime_put_autosuspend(priv->pm_dev);

    } else {
        /* 同步释放PM引用，强制立即断电休眠 */
        pm_runtime_put_sync(priv->pm_dev);
    }
    
    /* 返回写入字节数 */
    return count;
}

/* 定义sysfs设备属性 */
static DEVICE_ATTR_RW(power_state);

/* 注册属性到属性组 */
static struct attribute *regulator_sysfs_attrs[] = {
    &dev_attr_power_state.attr,
    NULL,
};

/* 定义属性组 */
static const struct attribute_group regulator_sysfs_attr_group = {
    .attrs = regulator_sysfs_attrs,
};

/*
 * 函数功能：平台设备探测函数
 * 作用：初始化资源 -> 获取Regulator -> 创建sysfs -> 初始化RPM
 */
static int regulator_probe(struct platform_device *pdev)
{
    /* 定义函数返回值变量 */
    int ret;
    /* 定义驱动私有结构体指针 */
    struct regulator_rpm_dev *priv;

    printk(KERN_INFO "regulator_rpm driver probe\n");

    /*  devm托管分配私有结构体内存 */
    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    /* 初始化互斥锁 */
    mutex_init(&priv->lock);

    /* PM操作设备指针指向平台设备，统一RPM操作对象 */
    priv->pm_dev = &pdev->dev;

    /* 从设备树获取Regulator消费者句柄，id为vcc */
    priv->regulator = devm_regulator_get(&pdev->dev, "vcc");
    if (IS_ERR(priv->regulator)) {
        ret = PTR_ERR(priv->regulator);
        dev_err(&pdev->dev, "failed to get regulator\n");
        return ret;
    }

    /* 驱动初始化时主动使能电源，确保硬件初始上电 */
    ret = regulator_enable(priv->regulator);
    if (ret) {
        dev_err(&pdev->dev, "failed to enable regulator at probe\n");
        return ret;
    }
    
    /* 同步状态缓存为开启状态 */
    priv->is_on = 1;

    /* 创建sysfs类 */
    priv->class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(priv->class)) {
        regulator_disable(priv->regulator);
        return PTR_ERR(priv->class);
    }

    /* 在sysfs类下创建设备节点 */
    priv->sysfs_dev = device_create(priv->class, &pdev->dev,
                                    MKDEV(0, 0), NULL, DEV_NAME);
    if (IS_ERR(priv->sysfs_dev)) {
        regulator_disable(priv->regulator);
        goto destroy_class;
    }

    /* 将私有数据绑定到sysfs设备 */
    dev_set_drvdata(priv->sysfs_dev, priv);
    /* 将私有数据绑定到平台设备 */
    dev_set_drvdata(&pdev->dev, priv);

    /* 创建sysfs属性组 */
    ret = sysfs_create_group(&priv->sysfs_dev->kobj, &regulator_sysfs_attr_group);
    if (ret) {
        goto destroy_device;
    }

    /* 标记设备初始状态为活跃，初始化RPM状态 */
    pm_runtime_set_active(&pdev->dev);

    /* 使能Runtime PM功能 */
    pm_runtime_enable(&pdev->dev);

    /* 设置自动挂起延迟时间为1000ms */
    pm_runtime_set_autosuspend_delay(&pdev->dev, 1000);

    /* 启用自动挂起功能 */
    pm_runtime_use_autosuspend(&pdev->dev);

    /* 释放PM初始引用，启动自动挂起计时 */
    pm_runtime_put(&pdev->dev);

    return 0;

destroy_device:
    /* 销毁sysfs设备节点 */
    device_destroy(priv->class, MKDEV(0, 0));

destroy_class:
    /* 销毁sysfs类 */
    class_destroy(priv->class);
    return ret;
}

/*
 * 函数功能：平台设备移除函数
 * 作用：安全关闭RPM -> 释放sysfs资源 -> 关闭电源
 */
static int regulator_remove(struct platform_device *pdev)
{
    /* 获取驱动私有结构体指针 */
    struct regulator_rpm_dev *priv = dev_get_drvdata(&pdev->dev);

    /* 同步唤醒设备，确保硬件处于活跃状态，安全卸载 */
    pm_runtime_get_sync(&pdev->dev);

    /* 禁用Runtime PM功能 */
    pm_runtime_disable(&pdev->dev);

    /* 标记设备为挂起状态 */
    pm_runtime_set_suspended(&pdev->dev);

    /* 删除sysfs属性组 */
    sysfs_remove_group(&priv->sysfs_dev->kobj, &regulator_sysfs_attr_group);

    /* 销毁sysfs设备节点 */
    device_destroy(priv->class, MKDEV(0, 0));

    /* 销毁sysfs类 */
    class_destroy(priv->class);

    /* 打印驱动移除完成信息 */
    printk(KERN_INFO "regulator_rpm driver removed\n");
    
    return 0;
}

/* 定义设备树匹配表 */
static const struct of_device_id regulator_of_match[] = {
    { .compatible = "fire,regulator_rpm" },
    { }
};

/* 导出设备树匹配表 */
MODULE_DEVICE_TABLE(of, regulator_of_match);

/* 平台驱动结构体 */
static struct platform_driver regulator_driver = {
    .probe   = regulator_probe,
    .remove  = regulator_remove,
    .driver = {
        .name           = "regulator_rpm",
        .of_match_table = regulator_of_match,
        /* 绑定Runtime PM操作集 */
        .pm             = &reg_pm_ops,
    },
};

module_platform_driver(regulator_driver);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("regulator_rpm module");
MODULE_LICENSE("GPL");