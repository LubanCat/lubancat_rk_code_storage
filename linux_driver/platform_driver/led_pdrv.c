#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/fs.h>

#define DEV_MAJOR 243
#define DEV_NAME  "led"

static struct class *led_test_class;

//结构体led_data来管理我们LED灯的硬件信息
struct led_data {
	unsigned int led_pin;

	unsigned int __iomem *va_DDR;
	unsigned int __iomem *va_DR;

	struct cdev led_cdev;

};

static int led_cdev_open(struct inode *inode, struct file *filp)
{
	unsigned int val = 0;
	struct led_data *cur_led = container_of(inode->i_cdev, struct led_data, led_cdev);


	printk("led_cdev_open() \n");

	// 设置引脚输出
	val = readl(cur_led->va_DDR);
	val |= ((unsigned int)0X1 << (cur_led->led_pin+16));
	val |= ((unsigned int)0X1 << (cur_led->led_pin));
	writel(val,cur_led->va_DDR);

	//设置默认输出高电平
	val = readl(cur_led->va_DR);
	val |= ((unsigned int)0X1 << (cur_led->led_pin+16));
	val |= ((unsigned int)0x1 << (cur_led->led_pin));
	writel(val, cur_led->va_DR);

	filp->private_data = cur_led;

	return 0;
}


static int led_cdev_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t led_cdev_write(struct file *filp, const char __user * buf,
			      size_t count, loff_t * ppos)
{
	unsigned long val = 0;
	unsigned long ret = 0;

	int tmp = count;

	struct led_data *cur_led = (struct led_data *)filp->private_data;

	val = kstrtoul_from_user(buf, tmp, 10, &ret);

	val = readl(cur_led->va_DR);
	if (ret == 0)
	{
		val |= ((unsigned int)0x1 << ((cur_led->led_pin)+16));
		val &= ~((unsigned int)0X1 << (cur_led->led_pin));
	}
	else
	{
		val |= ((unsigned int)0x1 << (cur_led->led_pin+16));
		val |= ((unsigned int)0X1 << (cur_led->led_pin));
	}
	writel(val, cur_led->va_DR);

	*ppos += tmp;

	return tmp;
}

static struct file_operations led_cdev_fops = {
	.open = led_cdev_open,
	.release = led_cdev_release,
	.write = led_cdev_write,
};


//probe函数中,驱动需要去提取设备的资源,完成字符设备的注册等工作
static int led_pdrv_probe(struct platform_device *pdev)
{
	struct led_data *cur_led;
	unsigned int *led_hwinfo;
	
	struct resource *mem_DR;
	struct resource *mem_DDR;

	dev_t cur_dev;

	int ret = 0;
	
	printk("led platform driver probe\n");

	//第一步：提取平台设备提供的资源
	//devm_kzalloc函数申请cur_led和led_hwinfo结构体内存大小
	cur_led = devm_kzalloc(&pdev->dev, sizeof(struct led_data), GFP_KERNEL);
	if(!cur_led)
		return -ENOMEM;
	led_hwinfo = devm_kzalloc(&pdev->dev, sizeof(unsigned int)*2, GFP_KERNEL);
	if(!led_hwinfo)
		return -ENOMEM;

	/* get the pin for led and the reg's shift */
	//dev_get_platdata函数获取私有数据，得到LED灯的寄存器偏移量，并赋值给cur_led->led_pin
	led_hwinfo = dev_get_platdata(&pdev->dev);

	cur_led->led_pin = led_hwinfo[0];
	/* get platform resource */
	//利用函数platform_get_resource可以获取到各个寄存器的地址
	mem_DR = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mem_DDR = platform_get_resource(pdev, IORESOURCE_MEM, 1);

	//使用devm_ioremap将获取到的寄存器地址转化为虚拟地址
	cur_led->va_DR = devm_ioremap(&pdev->dev, mem_DR->start, resource_size(mem_DR));
	cur_led->va_DDR = devm_ioremap(&pdev->dev, mem_DDR->start, resource_size(mem_DDR));

	//第二步：注册字符设备
	cur_dev = MKDEV(DEV_MAJOR, pdev->id);

	register_chrdev_region(cur_dev, 1, "led_cdev");

	cdev_init(&cur_led->led_cdev, &led_cdev_fops);

	ret = cdev_add(&cur_led->led_cdev, cur_dev, 1);
	if(ret < 0)
	{
		printk("fail to add cdev\n");
		goto add_err;
	}
	
	device_create(led_test_class, NULL, cur_dev, NULL, DEV_NAME "%d", pdev->id);

	/* save as drvdata */ 
	//platform_set_drvdata函数，将LED数据信息存入在平台驱动结构体中pdev->dev->driver_data中
	platform_set_drvdata(pdev, cur_led);

	return 0;

add_err:
	unregister_chrdev_region(cur_dev, 1);
	return ret;
}


static int led_pdrv_remove(struct platform_device *pdev)
{
	dev_t cur_dev; 
	//platform_get_drvdata，获取当前LED灯对应的结构体
	struct led_data *cur_data = platform_get_drvdata(pdev);


	printk("led platform driver remove\n");

	cur_dev = MKDEV(DEV_MAJOR, pdev->id);

	//cdev_del删除对应的字符设备
	cdev_del(&cur_data->led_cdev);

	//删除/dev目录下的设备
	device_destroy(led_test_class, cur_dev);

	//unregister_chrdev_region， 注销掉当前的字符设备编号
	unregister_chrdev_region(cur_dev, 1);

	return 0;
}

static struct platform_device_id led_pdev_ids[] = {
	{.name = "led_pdev"},
	{}
};

MODULE_DEVICE_TABLE(platform, led_pdev_ids);

//平台总线匹配过程中 ，只会根据id_table中的name值进行匹配，若和平台设备的name值相等，则表示匹配成功； 反之，则匹配不成功，表明当前内核没有该驱动能够支持的设备。
static struct platform_driver led_pdrv = {
	
	.probe = led_pdrv_probe,
	.remove = led_pdrv_remove,
	.driver.name = "led_pdev",
	.id_table = led_pdev_ids,
};


static __init int led_pdrv_init(void)
{
	printk("led platform driver init\n");
	//class_create，来创建一个led类
	led_test_class = class_create(THIS_MODULE, "test_leds");
	//调用函数platform_driver_register，注册我们的平台驱动结构体，这样当加载该内核模块时， 就会有新的平台驱动加入到内核中。 第20-27行，注销
	platform_driver_register(&led_pdrv);

	return 0;
}
module_init(led_pdrv_init);


static __exit void led_pdrv_exit(void)
{
	printk("led platform driver exit\n");	
	platform_driver_unregister(&led_pdrv);
	class_destroy(led_test_class);
}
module_exit(led_pdrv_exit);

MODULE_AUTHOR("Embedfire");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("the example for platform driver");
