#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h> /* 平台设备*/
#include <linux/gpio/consumer.h> /* GPIO描述符*/
#include <linux/of.h> /* DT*/
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#include <linux/fs.h>
#include <linux/ide.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <asm/io.h>

/*------------------字符设备内容----------------------*/
#define DEV_NAME            "led_test"
#define DEV_CNT                 (1)

/*定义字符设备的设备号*/
static dev_t led_devno;

/*定义字符设备结构体chr_dev  */
static struct cdev led_chr_dev;

struct class *class_led;        //保存创建的类
struct device *device;      // 保存创建的设备


static struct gpio_desc *led;

static const struct of_device_id dt_ids[] =
{
	{ .compatible = "fire,led_test", },
	{  }
};


/*字符设备操作函数集，open函数*/
static int led_chr_dev_open(struct inode *inode, struct file *filp)
{
	pr_info("device probed!\n");
    	return 0;
}

/*字符设备操作函数集，write函数*/
static ssize_t led_chr_dev_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{

        unsigned char write_data; //用于保存接收到的数据

        int error = copy_from_user(&write_data, buf, cnt);
        if(error < 0) {
                printk("led_test write failed \n");
                return -1;
        }

    /*设置GPIO 输出电平 具体引脚根据板卡原理图*/
        if(write_data)
        {
                gpiod_set_value(led,1);  // 引脚输出高电平，红灯灭
        }
        else
        {
                gpiod_set_value(led, 0);    // 引脚输出底电平，红灯亮
        }

        return 0;
}

/*字符设备操作函数集*/
static struct file_operations  led_chr_dev_fops = 
{
        .owner = THIS_MODULE,
        .open = led_chr_dev_open,
        .write = led_chr_dev_write,
};


static int my_pdrv_probe (struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;

	/*gpio申请，指定输出*/

	led = gpiod_get(dev, "led", GPIOD_OUT_HIGH);

	/*---------------------注册 字符设备部分-----------------*/

        //第一步
    	//采用动态分配的方式，获取设备编号，次设备号为0，
    	//设备名称为led_test，可通过命令cat  /proc/devices查看
    	//DEV_CNT为1，当前只申请一个设备编号
    	ret = alloc_chrdev_region(&led_devno, 0, DEV_CNT, DEV_NAME);
    	if(ret < 0){
        	printk("fail to alloc led_devno\n");
        	goto alloc_err;
    	}
    	//第二步
    	//关联字符设备结构体cdev与文件操作结构体file_operations
        led_chr_dev.owner = THIS_MODULE;
    	cdev_init(&led_chr_dev, &led_chr_dev_fops);
    	//第三步
    	//添加设备至cdev_map散列表中
    	ret = cdev_add(&led_chr_dev, led_devno, DEV_CNT);
    	if(ret < 0)
    	{
        	printk("fail to add cdev\n");
        	goto add_err;
    	}

        //第四步
        /*创建类 */
        class_led = class_create(THIS_MODULE, DEV_NAME);

        /*创建设备*/
        device = device_create(class_led, NULL, led_devno, NULL, DEV_NAME);

	pr_info("device probed!\n");
	return 0;

add_err:
    //添加设备失败时，需要注销设备号
    unregister_chrdev_region(led_devno, DEV_CNT);
    printk("error! \n");
alloc_err:

        return -1;
}


static struct platform_driver mypdrv = {
	.probe = my_pdrv_probe,
	.driver = {
		.name = "pdrv_led_test",
		.of_match_table = dt_ids,
		.owner = THIS_MODULE,
	},
};

/*驱动初始化函数*/
static int __init led_platform_driver_init(void)
{

        platform_driver_register(&mypdrv);

        return 0;
}

/*驱动注销函数*/
static void __exit led_platform_driver_exit(void)
{
        pr_info("led_test exit!\n");

	gpiod_put(led);

        /*删除设备*/
        device_destroy(class_led, led_devno);             //清除设备
        class_destroy(class_led);                                         //清除类
        cdev_del(&led_chr_dev);                                           //清除设备号
        unregister_chrdev_region(led_devno, DEV_CNT); //取消注册字符设备

        platform_driver_unregister(&mypdrv);
}


module_init(led_platform_driver_init);
module_exit(led_platform_driver_exit);

MODULE_LICENSE("GPL");




