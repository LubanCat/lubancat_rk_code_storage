/*
 * Copyright (C) 2022 - All Rights Reserved by
 * EmbedFire LubanCat
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include "spi_oled.h"

/*------------------字符设备内容----------------------*/
#define DEV_NAME "spi_oled"
#define DEV_CNT (1)

static dev_t oled_devno;		 //定义字符设备的设备号
static struct cdev oled_chr_dev; //定义字符设备结构体chr_dev
struct class *class_oled;		 //保存创建的类
struct device *device_oled;		 // 保存创建的设备

/*------------------IIC设备内容----------------------*/
struct spi_device	*oled_spi_device; //保存oled设备对应的spi_device结构体，匹配成功后由.prob函数带回。
struct device_node *oled_device_node;	   //spi_oled的设备树节点结构体
int oled_control_pin_number;			   // 保存oled D/C控制引脚的引脚号（从对应设备树节点中获取）
u8 oled_init_data[] = {
	0xae, 0xae, 0x00, 0x10, 0x40,
	0x81, 0xcf, 0xa1, 0xc8, 0xa6,
	0xa8, 0x3f, 0xd3, 0x00, 0xd5,
	0x80, 0xd9, 0xf1, 0xda, 0x12,
	0xdb, 0x40, 0x20, 0x02, 0x8d,
	0x14, 0xa4, 0xa6, 0xaf};

/*
*函数功能：向oled发送一个命令
*spi_device oled设备驱动对应的spi_device结构体。
*command  要发送的数据。
*返回值：成功，返回0 失败返回负数。
*/
static int oled_send_command(struct spi_device *spi_device, u8 command)
{
	int error = 0;
	u8 tx_data = command;
	struct spi_message message;   //定义发送的消息
	struct spi_transfer *transfer; //定义传输结构体

	/*设置 D/C引脚为低电平*/
	gpio_direction_output(oled_control_pin_number, 0);

	/*申请空间*/
	transfer = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);

	/*填充message和transfer结构体*/
	transfer->tx_buf = &tx_data;
	transfer->len = 1;
	spi_message_init(&message);
	spi_message_add_tail(transfer, &message);

	error = spi_sync(spi_device, &message);
	kfree(transfer);
	if (error != 0)
	{
		printk("spi_sync error! \n");
		return -1;
	}
	gpio_direction_output(oled_control_pin_number, 1);
	return 0;
}


/*
*函数功能：向oled发送一组命令
*spi_device oled设备驱动对应的spi_device结构体。
*commands  要发送的数据。
*返回值：成功，返回0 失败返回负数。
*/
static int oled_send_commands(struct spi_device *spi_device, u8 *commands, u16 lenght)
{
	int error = 0;
	int index ;
	
	for(index = 0; index < lenght; index++)
	{
		error = oled_send_command(spi_device, *(commands++));
		if(error < 0)
			return -1;
	}
	
	return 0;
}

/*
*向 oled 发送一个字节
*spi_device，指定oled 设备驱动的spi 结构体
*data, 要发送数据
*/
static int oled_send_one_u8(struct spi_device *spi_device, u8 data)
{
	int error = 0;
	u8 tx_data = data;
	struct spi_message message;   //定义发送的消息
	struct spi_transfer *transfer; //定义传输结构体

	/*设置 D/C引脚为高电平*/
	gpio_direction_output(oled_control_pin_number, 1);

	/*申请空间*/
	transfer = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);

	/*填充message和transfer结构体*/
	transfer->tx_buf = &tx_data;
	transfer->len = 1;
	spi_message_init(&message);
	spi_message_add_tail(transfer, &message);

	error = spi_sync(spi_device, &message);
	kfree(transfer);
	if (error != 0)
	{
		printk("spi_sync error! \n");
		return -1;
	}
	return 0;
}


/*
*向 oled 发送数据
*spi_device，指定oled 设备驱动的spi 结构体
*data, 要发送数据的地址
*lenght，发送的数据长度
*/
static int oled_send_data(struct spi_device *spi_device, u8 *data, u16 lenght)
{
	int error = 0;
	int index ;
	
	for(index = 0; index < lenght; index++)
	{
		error = oled_send_one_u8(spi_device, *(data++));
		if(error < 0)
			return -1;
	}
	
	return 0;
}

/*
* 填充整个OLED 显示屏 
* bmp_dat， 填充的值 
*/
void oled_fill(unsigned char bmp_dat)
{
	u8 y, x;
	for (y = 0; y < 8; y++)
	{
		oled_send_command(oled_spi_device, 0xb0 + y);
		oled_send_command(oled_spi_device, 0x01);
		oled_send_command(oled_spi_device, 0x10);
		for (x = 0; x < 128; x++)
		{
			oled_send_one_u8(oled_spi_device, bmp_dat);
		}
	}
}


/*
*向 oled 发送要显示的数据，x, y 指定显示的起始位置，支持自动换行
*spi_device，指定oled 设备驱动的spi 结构体
*display_buffer, 数据地址
*x, y,起始坐标。
*length， 发送长度
*/
static int oled_display_buffer(u8 *display_buffer, u8 x, u8 y, u16 length)
{
	u16 index = 0;
	int error = 0;

	do
	{
		/*设置写入的起始坐标*/
		error += oled_send_command(oled_spi_device, 0xb0 + y);
		error += oled_send_command(oled_spi_device, ((x & 0xf0) >> 4) | 0x10);
		error += oled_send_command(oled_spi_device, (x & 0x0f) | 0x01);

		if (length > (X_WIDTH - x))
		{
			error += oled_send_data(oled_spi_device, display_buffer + index, X_WIDTH - x);
			length -= (X_WIDTH - x);
			index += (X_WIDTH - x);
			x = 0;
			y++;
		}
		else
		{
			error += oled_send_data(oled_spi_device, display_buffer + index, length);
			index += length;
			// x += length;
			length = 0;
		}

	} while (length > 0);

	if (error != 0)
	{
		/*发送错误*/
		printk("oled_display_buffer error! %d \n",error);
		return -1;
	}
	return index;
}


/*oled 初始化函数*/
void spi_oled_init(void)
{
	/*初始化oled*/
	oled_send_commands(oled_spi_device, oled_init_data, sizeof(oled_init_data));

	/*清屏*/
	oled_fill(0x00);
}

/*字符设备操作函数集，open函数实现*/
static int oled_open(struct inode *inode, struct file *filp)
{
	spi_oled_init(); //初始化显示屏
	return 0;
}


/*字符设备操作函数集，.write函数实现*/
static ssize_t oled_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *off)
{
	int copy_number=0;

	/*申请内存*/
	oled_display_struct *write_data;
	write_data = (oled_display_struct*)kzalloc(cnt, GFP_KERNEL);

	copy_number = copy_from_user(write_data, buf,cnt);
	oled_display_buffer(write_data->display_buffer, write_data->x, write_data->y, write_data->length);

	/*释放内存*/
	kfree(write_data);
	return 0;
}


/*字符设备操作函数集，.release函数实现*/
static int oled_release(struct inode *inode, struct file *filp)
{
	oled_send_command(oled_spi_device, 0xae);//关闭显示
	return 0;
}


/*字符设备操作函数集*/
static struct file_operations oled_chr_dev_fops = {
	.owner = THIS_MODULE,
	.open = oled_open,
	.write = oled_write,
	.release = oled_release};

/*----------------平台驱动函数集-----------------*/
static int oled_probe(struct spi_device *spi)
{

	struct device_node *node=spi->dev.of_node;

	printk(KERN_EMERG " match successed \n");

	/*获取 oled 的 D/C 控制引脚并设置为输出，默认高电平*/
	oled_control_pin_number = of_get_named_gpio(node, "dc_control_pin", 0);
	
	printk("oled_control_pin_number = %d,\n ", oled_control_pin_number);

	gpio_request(oled_control_pin_number, "dc_control_pin");
	gpio_direction_output(oled_control_pin_number, 1);

	/*初始化spi*/
	oled_spi_device = spi;
	oled_spi_device->mode = SPI_MODE_0;
	oled_spi_device->max_speed_hz = 2000000;
	spi_setup(oled_spi_device);

	/*打印oled_spi_device 部分内容*/
	printk("max_speed_hz = %d\n", oled_spi_device->max_speed_hz);
	printk("chip_select = %d\n", (int)oled_spi_device->chip_select);
	printk("bits_per_word = %d\n", (int)oled_spi_device->bits_per_word);
	printk("mode = %02X\n", oled_spi_device->mode);
	printk("cs_gpio = %02X\n", oled_spi_device->cs_gpio);

	return 0;

}

static int oled_remove(struct spi_device *spi)
{
	return 0;
}

/*传统的方式指定 ID 匹配表*/
static const struct spi_device_id oled_device_id[] = {
	{"fire,spi_oled", 0},
	{}};

/*指定设备树匹配表*/
static const struct of_device_id oled_of_match_table[] = {
	{.compatible = "fire,spi_oled"},
	{}};

/*spi 总线设备结构体*/
struct spi_driver oled_driver = {
	.probe = oled_probe,
	.remove = oled_remove,
	.id_table = oled_device_id,
	.driver = {
		.name = "spi_oled",
		.of_match_table = of_match_ptr(oled_of_match_table),
	},
};

/*
*驱动初始化函数
*/
static int __init oled_driver_init(void)
{
	int error;
	int ret = -1; //保存错误状态码
	pr_info("oled_driver_init\n");

	/*---------------------注册 字符设备部分-----------------*/

	//采用动态分配的方式，获取设备编号，次设备号为0，
	//设备名称spi_oled，可通过命令cat  /proc/devices查看
	//DEV_CNT为1，当前只申请一个设备编号
	ret = alloc_chrdev_region(&oled_devno, 0, DEV_CNT, DEV_NAME);
	if (ret < 0)
	{
		printk("fail to alloc oled_devno\n");
		goto alloc_err;
	}

	//关联字符设备结构体cdev与文件操作结构体file_operations
	oled_chr_dev.owner = THIS_MODULE;
	cdev_init(&oled_chr_dev, &oled_chr_dev_fops);

	// 添加设备至cdev_map散列表中
	ret = cdev_add(&oled_chr_dev, oled_devno, DEV_CNT);
	if (ret < 0)
	{
		printk("fail to add cdev\n");
		goto add_err;
	}

	/*创建类 */
	class_oled = class_create(THIS_MODULE, DEV_NAME);

	/*创建设备 DEV_NAME 指定设备名，*/
	device_oled = device_create(class_oled, NULL, oled_devno, NULL, DEV_NAME);

	error = spi_register_driver(&oled_driver);
	if (error < 0) {
		device_destroy(class_oled, oled_devno);		   //清除设备
		class_destroy(class_oled);					   //清除类
		cdev_del(&oled_chr_dev);					   //清除设备号
		unregister_chrdev_region(oled_devno, DEV_CNT); //取消注册字符设备
	}
	return error;

add_err:
	// 添加设备失败时，需要注销设备号
	unregister_chrdev_region(oled_devno, DEV_CNT);
	printk(" error! \n");
alloc_err:

	return -1;

}

/*
*驱动注销函数
*/
static void __exit oled_driver_exit(void)
{
	pr_info("oled_driver_exit\n");

	spi_unregister_driver(&oled_driver);
	gpio_free(oled_control_pin_number);

	/*删除设备*/
	device_destroy(class_oled, oled_devno);		   //清除设备
	class_destroy(class_oled);					   //清除类
	cdev_del(&oled_chr_dev);					   //清除设备号
	unregister_chrdev_region(oled_devno, DEV_CNT); //取消注册字符设备
}

module_init(oled_driver_init);
module_exit(oled_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedfire");
