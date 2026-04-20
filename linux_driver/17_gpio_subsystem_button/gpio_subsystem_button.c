#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/fs.h>

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>

#include <linux/spinlock.h>
#include <linux/timer.h>

/* 定义字符设备的名称 */
#define DEV_NAME    "gpio_subsystem_button"
/* 定义字符设备的数量 */
#define DEV_CNT     1

/* 定义设备号变量 */
static dev_t devno;
/* 定义设备类指针 */
static struct class *class;
/* 定义设备指针 */
static struct device *device;

/* 定义 LED 字符设备结构体 */
struct led_chrdev {
    /* 字符设备结构体 */
    struct cdev dev;
    /* 自旋锁 */
    spinlock_t spinlock;
    /* 定时器 */
    struct timer_list timer;
    /* LED 状态 */
    int led_state;
    /* 定时器间隔时间 */
    unsigned long timer_interval;
    /* LED 的 GPIO 描述符 */
    struct gpio_desc *led_gpio;
    /* 按钮的 GPIO 描述符 */
    struct gpio_desc *button_gpio;
    /* 按键上一次的状态 */
    int last_button_state;
};

/* 定义 led_chrdev 结构体指针，用于动态分配内存管理 LED 硬件信息 */
static struct led_chrdev *led_cdev;

/* 定时器回调函数 */
static void led_timer_callback(struct timer_list *t)
{   
    /* 反向找到包含这个定时器的设备结构体 led_chrdev 指针 */
    struct led_chrdev *led_cdev = from_timer(led_cdev, t, timer);

    /* 用于保存中断状态信息 */
    unsigned long flags;

    // 标记是否发生按键状态变化
    int btn_changed = 0;

    // 记录按键动作 0=按下 1=松开
    int btn_action = 0;

    /* 获取自旋锁并保存中断状态 */
    spin_lock_irqsave(&led_cdev->spinlock, flags);

    /* 读取当前按键的电平状态 */
    int current_button_state = gpiod_get_value(led_cdev->button_gpio);

    /* 检查当前按键状态是否与上一次记录的状态不同 */
    if (current_button_state != led_cdev->last_button_state) {
        /* 标记状态改变 */
        btn_changed = 1;
        /* 记录当前状态 */
        btn_action = current_button_state;

        /* 当 current_button_state 为 0 时，表示检测到按键被按下 */
        if (current_button_state == 0) { 
            /* 翻转 LED 电平 */
            gpiod_set_value(led_cdev->led_gpio, !gpiod_get_value(led_cdev->led_gpio));
        }

        /* 更新按键上一次的状态，以便在下一次定时器回调时作为上一次的状态进行比较 */
        led_cdev->last_button_state = current_button_state;
    }

    /* 重新启动定时器 */
    mod_timer(&led_cdev->timer, jiffies + led_cdev->timer_interval);

    /* 释放自旋锁并恢复中断状态 */
    spin_unlock_irqrestore(&led_cdev->spinlock, flags);

    /* 打印按键是否按下或松开 */
    if (btn_changed) {
        if (btn_action == 0) {
            printk(KERN_INFO "按键已按下\n");
        } else {
            printk(KERN_INFO "按键已松开\n");
        }
    }
}

/* 
 * pdrv_led_open 函数是字符设备的打开操作函数。
 * 当用户空间程序打开该设备节点时，会调用此函数。
 */
static int pdrv_led_open(struct inode *inode, struct file *filp)
{
    /* 通过 inode 中的 i_cdev 成员找到对应的 led_chrdev 结构体 */
    struct led_chrdev *led_cdev = container_of(inode->i_cdev, struct led_chrdev, dev);
    /* 将 led_chrdev 结构体指针存储到文件的私有数据中 */
    filp->private_data = led_cdev;

    /* 打印设备打开信息 */
    printk("pdrv_led open\n");

    return 0;
}

/* 字符设备释放函数 */
static int pdrv_led_release(struct inode *inode, struct file *filp)
{
    printk("pdrv_led release\r\n");
    return 0;
}

/* 
 * pdrv_led_write 函数是字符设备的写操作函数。
 * 当用户空间程序向该设备节点写入数据时，会调用此函数。
 * 功能：根据用户写入的数据以及按键状态控制 LED 灯的亮灭。
 */
static ssize_t pdrv_led_write(struct file *filp, const char __user *buf,
                              size_t count, loff_t *ppos)
{
    /* 用于存储用户输入 */
    char input[32] = {0};
    /* 用于存储用户输入的时间参数 */
    unsigned long interval = 0;
    unsigned long val = 0;
    /* 从文件结构体的私有数据中获取 led_chrdev 结构体指针 */
    struct led_chrdev *led_cdev = filp->private_data;
    /* 用于保存中断状态信息 */
    unsigned long flags;

    /* 打印设备写操作信息 */
    printk("pdrv_led write \r\n");

    /* 从用户空间读取输入 */
    if (copy_from_user(input, buf, min(count, sizeof(input) - 1))) {
        return -EFAULT;
    }

    /* 解析用户输入 */
    if (sscanf(input, "%lu", &val) >= 1) {

    printk("val = %ld \n", val);

    /* 获取自旋锁并保存中断状态 */
    spin_lock_irqsave(&led_cdev->spinlock, flags);
    
    switch (val) {
        /* 关闭 LED */
        case 0:
            led_cdev->led_state = 0;
            /* 停止定时器 */
            del_timer_sync(&led_cdev->timer);
            /* 设置 LED 为高电平 */
            gpiod_set_value(led_cdev->led_gpio, 1);
            break;
        /* 按键控制LED亮灭 */
        case 1:
            led_cdev->led_state = 1;
            /* 启动定时器 */
            mod_timer(&led_cdev->timer, jiffies + led_cdev->timer_interval);
            break;
        /* 修改定时器时间 */
        case 2:
            if (sscanf(input, "%lu %lu", &val, &interval) == 2 && interval > 0) {
                /* 转换为 jiffies */
                led_cdev->timer_interval = msecs_to_jiffies(interval);
                printk("Timer interval set to %lu ms\n", interval);
            } else {
                printk(KERN_ERR "Invalid interval value\n");
            }
            break;

        default:
            /* 释放自旋锁并恢复中断状态 */
            spin_unlock_irqrestore(&led_cdev->spinlock, flags);
            return -EINVAL;
        }
    }

    /* 释放自旋锁并恢复中断状态 */
    spin_unlock_irqrestore(&led_cdev->spinlock, flags);

    return count;
}

/* 字符设备读函数 */
static ssize_t pdrv_led_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    return 0;
}

/* 定义字符设备的文件操作结构体 */
static struct file_operations pdrv_led_fops = {
    .owner = THIS_MODULE,
    .open = pdrv_led_open,
    .release = pdrv_led_release,
    .write = pdrv_led_write,
    .read = pdrv_led_read,
};

/* 
 * pdrv_led_probe 函数是平台驱动的探测函数。
 * 功能：提取平台设备或设备树的资源，设置LED引脚为输出模式，并默认输出高电平，完成字符设备的注册和初始化。
 */
static int pdrv_led_probe(struct platform_device *pdev)
{
    /* 定义返回值变量 */
    int ret = 0;
    /* 定义主设备号变量 */
    int major;
    /* 定义次设备号变量 */
    int minor;

    /* 打印平台驱动探测信息 */
    printk("led platform driver probe\n");

    /* 第一步：提取平台设备提供的资源 */
    /* 使用 devm_kzalloc 函数为 led_chrdev 结构体分配内存并清零 */
    led_cdev = devm_kzalloc(&pdev->dev, sizeof(struct led_chrdev), GFP_KERNEL);
    if (!led_cdev)
        return -ENOMEM;

    if (pdev->dev.of_node) {
        /* 获取 LED 的 GPIO 描述符，GPIOD_OUT_HIGH 表示设置为输出模式 + 输出高电平 */
        led_cdev->led_gpio = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_HIGH);
        if (IS_ERR(led_cdev->led_gpio)) {
            ret = PTR_ERR(led_cdev->led_gpio);
            printk(KERN_ERR "Failed to get LED GPIO: %d\n", ret);
            return ret;
        }

        /* 获取按钮的 GPIO 描述符，GPIOD_IN 表示设置为输入模式 */
        led_cdev->button_gpio = devm_gpiod_get(&pdev->dev, "button", GPIOD_IN);
        if (IS_ERR(led_cdev->button_gpio)) {
            ret = PTR_ERR(led_cdev->button_gpio);
            printk(KERN_ERR "Failed to get button GPIO: %d\n", ret);
            return ret;
        }
        
    } else {
        printk("Platform device matching is not supported in this driver\n");
        return -ENOMEM;
    }

    /* 第二步：初始化注册字符设备 */
    /* 分配设备号 */
    ret = alloc_chrdev_region(&devno, 0, DEV_CNT, DEV_NAME);
    if (ret < 0) {
        /* 打印设备号分配失败信息 */
        printk("fail to alloc devno\n");
        /* 跳转到错误处理标签 */
        goto ioremap_err;
    }
    /* 获取主设备号 */
    major = MAJOR(devno);
    /* 获取次设备号 */
    minor = MINOR(devno);
    /* 打印主设备号和次设备号 */
    printk("major=%d, minor=%d\n", major, minor);

    /* 初始化字符设备 */
    cdev_init(&led_cdev->dev, &pdrv_led_fops);
    led_cdev->dev.owner = THIS_MODULE;

    /* 添加字符设备 */
    ret = cdev_add(&led_cdev->dev, devno, DEV_CNT);
    if (ret < 0) {
        /* 打印字符设备添加失败信息 */
        printk("fail to add cdev\n");
        /* 跳转到错误处理标签 */
        goto add_err;
    }

    /* 创建设备类 */
    class = class_create(THIS_MODULE, DEV_NAME);
    if (IS_ERR(class)) {
        /* 打印设备类创建失败信息 */
        printk("fail to create class\n");
        ret = PTR_ERR(class);
        /* 跳转到错误处理标签 */
        goto class_err;
    }

    /* 创建设备节点 */
    device = device_create(class, NULL, devno, NULL, DEV_NAME);
    if (IS_ERR(device)) {
        /* 打印设备节点创建失败信息 */
        printk("fail to create device\n");
        ret = PTR_ERR(device);
        /* 跳转到错误处理标签 */
        goto device_err;
    }

    /* 保存驱动数据 */
    platform_set_drvdata(pdev, led_cdev);

    /* 初始化自旋锁 */
    spin_lock_init(&led_cdev->spinlock);

    /* 初始化定时器 */
    timer_setup(&led_cdev->timer, led_timer_callback, 0);
    /* timer_interval单位为jiffies
     * 当HZ=100时，此处定时器实际间隔时间为 HZ/2 个 jiffies，即 HZ/2 * (1/HZ)= 0.5秒
     * 当HZ=300时，此处定时器实际间隔时间为 HZ/2 个 jiffies，即 HZ/2 * (1/HZ)= 1秒
     */
    led_cdev->timer_interval = HZ / 2;
    /* 默认关闭 LED */
    led_cdev->led_state = 0;

    /* 记录按键初始状态 */
    led_cdev->last_button_state = gpiod_get_value(led_cdev->button_gpio);

     /* 打印按键初始状态 */
    printk("last_button_state = %d\n", led_cdev->last_button_state);

    return 0;

device_err:
    /* 销毁设备类 */
    class_destroy(class);

class_err:
    /* 删除字符设备 */
    cdev_del(&led_cdev->dev);

add_err:
    /* 释放设备号 */
    unregister_chrdev_region(devno, DEV_CNT);

ioremap_err:
    return ret;
}

/* 
 * pdrv_led_remove 函数是平台驱动的移除函数。
 * 功能：释放字符设备相关资源，包括设备节点、设备类、字符设备和设备号等。
 */
static int pdrv_led_remove(struct platform_device *pdev)
{
    struct led_chrdev *led_cdev = platform_get_drvdata(pdev);

    /* 打印平台驱动移除信息 */
    printk("pdrv_led remove\n");

    /* 删除定时器 */
    del_timer_sync(&led_cdev->timer);

    /* 销毁设备节点 */
    device_destroy(class, devno);

    /* 删除字符设备 */
    cdev_del(&led_cdev->dev);

    /* 释放设备号 */
    unregister_chrdev_region(devno, DEV_CNT);

    /* 销毁设备类 */
    class_destroy(class);

    return 0;
}

/* 定义平台设备 ID 表，用于平台总线的匹配 */
static struct platform_device_id pdev_led_ids[] = {
    {.name = "pdev_led"},
    {}
};

/* 声明平台设备 ID 表，供内核使用 */
MODULE_DEVICE_TABLE(platform, pdev_led_ids);

static const struct of_device_id dts_led_of_match[] = {
    { .compatible = "fire,led_test" },
    {}
};

MODULE_DEVICE_TABLE(of, dts_led_of_match);

/* 定义平台驱动结构体，包含探测、移除函数，驱动名称和设备 ID 表 */
static struct platform_driver pdrv_led = {
    .probe = pdrv_led_probe,
    .remove = pdrv_led_remove,
    .id_table = pdev_led_ids,
    .driver     = {
        .name    = "dts_led",
        .of_match_table = dts_led_of_match,
    },
};

/* 
 * pdrv_led_init 函数是模块的初始化函数。
 * 功能：注册平台驱动。
 */
static __init int pdrv_led_init(void)
{
    /* 打印平台驱动初始化信息 */
    printk("led platform driver init\n");
    /* 注册平台驱动 */
    platform_driver_register(&pdrv_led);

    return 0;
}

/* 
 * pdrv_led_exit 函数是模块的退出函数。
 * 功能：注销平台驱动。
 */
static __exit void pdrv_led_exit(void)
{
    /* 打印平台驱动退出信息 */
    printk("led platform driver exit\n");
    /* 注销平台驱动 */
    platform_driver_unregister(&pdrv_led);
}

module_init(pdrv_led_init);
module_exit(pdrv_led_exit);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("gpio_subsystem_button module");
MODULE_LICENSE("GPL");