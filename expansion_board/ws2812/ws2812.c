#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/delay.h>

#define DEV_NAME            "ws2812"
#define DEV_CNT                 (1)

#define GPIO3_BASE (0xfe760000)
//每组GPIO,有2个寄存器,对应32个引脚，每个寄存器负责16个引脚；
//一个寄存器32位，其中高16位都是使能位，低16位对应16个引脚，每个引脚占用1比特位
#define GPIO3_DR_L (GPIO3_BASE + 0x0000)
#define GPIO3_DR_H (GPIO3_BASE + 0x0004)
#define GPIO3_DDR_L (GPIO3_BASE + 0x0008)
#define GPIO3_DDR_H (GPIO3_BASE + 0x000C)

#define SYS_GRF_BASE (0xFDC60000)
#define GRF_GPIO3C_P (SYS_GRF_BASE + 0x00A8) //上下拉
#define GRF_GPIO3C_SL (SYS_GRF_BASE + 0x01A8) //转换速率

static dev_t devno;
struct class *led_chrdev_class;
static void led_res(struct file *filp);

struct led_chrdev {
	struct cdev dev;
	unsigned int __iomem *va_dr; 	// 数据寄存器，设置输出的电压
	unsigned int __iomem *va_ddr; 	// 数据方向寄存器，设置输入或者输出
	unsigned int __iomem *va_sl; 	// 转换驱动速率
	unsigned int __iomem *va_p; 	// 上下拉

	unsigned int led_pin; // 偏移
};

static int led_chrdev_open(struct inode *inode, struct file *filp)
{	
	unsigned int val = 0;
	struct led_chrdev *led_cdev = (struct led_chrdev *)container_of(inode->i_cdev, struct led_chrdev,dev);
	filp->private_data = container_of(inode->i_cdev, struct led_chrdev, dev);

	//设置输出模式
	val = *(led_cdev->va_ddr);
	val |= ((unsigned int)0x1 << (led_cdev->led_pin+16));
	val |= ((unsigned int)0X1 << (led_cdev->led_pin));
	iowrite32(val,led_cdev->va_ddr);

	//设置上下拉
	//*(led_cdev[0].va_p)=(0xFFFFE66A);

	//设置转换驱动速率
	*(led_cdev[0].va_sl) = 0xFFFFFF3F;

	led_res(filp);
	
	return 0;
}

static void led_Write0(struct file *filp)
{	
	unsigned int val = 0;
	struct led_chrdev *led_cdev = (struct led_chrdev *)filp->private_data;

	val = *(led_cdev->va_dr);
	val |= 0xFFFF0008;
	*(led_cdev->va_dr) = val; //输出高电平
	*(led_cdev->va_dr) = val; //输出高电平
	*(led_cdev->va_dr) = val; //输出高电平  //执行3次330ns

    val &= ~(0x00000008);
	*(led_cdev->va_dr) =val;    /*设置GPIO引脚输出低电平*/
}

static void led_Write1(struct file *filp)
{	
	unsigned int val = 0;
	struct led_chrdev *led_cdev = (struct led_chrdev *)filp->private_data;
	val = *(led_cdev->va_dr);
	val |= 0xFFFF0008;
	*(led_cdev->va_dr) = val;  //输出高电平
	*(led_cdev->va_dr) = val;  //输出高电平
	*(led_cdev->va_dr) = val;  //输出高电平
	*(led_cdev->va_dr) = val;  //输出高电平
	*(led_cdev->va_dr) = val;  //输出高电平
	*(led_cdev->va_dr) = val;  //输出高电平
	*(led_cdev->va_dr) =val;    //输出高电平 //执行7次770ns

    val &= ~(0x00000008);
	*(led_cdev->va_dr) =val;    //输出低电平 //执行7次770ns

}

static void led_res(struct file *filp)
{	
	unsigned int val = 0;
	struct led_chrdev *led_cdev = (struct led_chrdev *)filp->private_data;

    val &= ~(0x00000008);
	*(led_cdev->va_dr) =val;    /*设置GPIO引脚输出低电平*/
	udelay(300);

	val = *(led_cdev->va_dr);
	val |= 0xFFFF0008;
	*(led_cdev->va_dr) = val; //输出高电平
	udelay(300);

	val &= ~(0x00000008);
	*(led_cdev->va_dr) =val;    /*设置GPIO引脚输出低电平*/

}

static void RGB_WByte(struct file *filp, uint8_t byte)
{
	uint8_t i=0;
	for(i=0;i<8;i++)
	{
	  if((byte<<i)&0x01)
		  led_Write1(filp);
	  else
		  led_Write0(filp);
	}

}

static void Write24Bit(struct file *filp, uint8_t green, uint8_t red, uint8_t blue)
{
	RGB_WByte(filp, green);
	RGB_WByte(filp, red);
	RGB_WByte(filp, blue);
}

static void RGB_LED_Red(struct file *filp)
{
	Write24Bit(filp, 0,0xff,0);
}

static void RGB_LED_Green(struct file *filp)
{
	Write24Bit(filp, 0xff,0,0);

}

static void RGB_LED_Blue(struct file *filp)
{
	Write24Bit(filp, 0,0,0xff);
}
static void RGB_LED_White(struct file *filp)
{
	Write24Bit(filp, 0xff,0xff,0xff);
}
static void RGB_LED_Orange(struct file *filp)
{
	Write24Bit(filp, 0xa5,0xff,0x00);;
}
static void RGB_LED_Yellow(struct file *filp)
{
	Write24Bit(filp, 0xff,0x00,0xa5);
}
static void RGB_LED_Purple(struct file *filp)
{
	Write24Bit(filp, 0x00,0xff,0xa5);
}

static ssize_t led_chrdev_write(struct file *filp, const char __user * buf,
                size_t count, loff_t * ppos)
{
	int i;
	char ret = 0;
	get_user(ret, buf);

	printk("write \n");

	if (ret == '0'){
		RGB_LED_Red(filp);
		printk("RGB_LED_Red \n");
	}
	else if (ret == '1'){
		RGB_LED_Green(filp);
		printk("RGB_LED_Green \n");
	}
	else if (ret == '2'){
		RGB_LED_Blue(filp);
		printk("RGB_LED_Blue \n");
	}
	else if (ret == '3'){
		RGB_LED_White(filp);
		printk("RGB_LED_White \n");
	}
	else if (ret == '4'){
		RGB_LED_Orange(filp);
		printk("RGB_LED_Orange \n");
	}
	else if (ret == '5'){
		RGB_LED_Yellow(filp);
		printk("RGB_LED_Yellow \n");
	}
	else if (ret == '6'){
		RGB_LED_Purple(filp);
		printk("RGB_LED_Purple \n");
	}
	else if (ret == '7'){
		for (i=0; i<30; i++){
		RGB_LED_Red(filp);
		RGB_LED_Green(filp);
		RGB_LED_Blue(filp);
		RGB_LED_White(filp);
		RGB_LED_Orange(filp);
		RGB_LED_Yellow(filp);
		RGB_LED_Purple(filp);
		mdelay(300);
		}
	}
	else if (ret == '8'){
		led_res(filp);
		printk("led_res \n");
	}
	else if (ret == 'a'){
		led_Write0(filp);
		led_Write0(filp);
		led_Write0(filp);
		led_Write0(filp);
		led_Write0(filp);
		led_Write0(filp);
		led_Write0(filp);
		led_Write0(filp);
		led_Write0(filp);
		led_res(filp);
	}
	else if (ret == 'b'){
		led_Write1(filp);
		led_Write1(filp);
		led_Write1(filp);
		led_Write1(filp);
		led_Write1(filp);
		led_Write1(filp);
		led_Write1(filp);
		led_Write1(filp);
		led_Write1(filp);
		led_Write1(filp);
		led_Write1(filp);
		led_Write1(filp);
		led_res(filp);
	}
	else {
		printk("请输入正确的灯状态!\n");
	}

	return count;
}

static int led_chrdev_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations led_chrdev_fops = {
	.owner = THIS_MODULE,
	.open = led_chrdev_open,
	.release = led_chrdev_release,
	.write = led_chrdev_write,
};

static struct led_chrdev led_cdev[DEV_CNT] = {
	{.led_pin = 3}, 	//偏移，高16引脚,GPIO3_C3
};

static __init int led_chrdev_init(void)
{
	int i = 0;
	dev_t cur_dev;

	printk("led_chrdev init (GPIO3_C3)\n");
	
	led_cdev[0].va_dr   = ioremap(GPIO3_DR_H, 4);	 //
	led_cdev[0].va_ddr  = ioremap(GPIO3_DDR_H, 4);	 // 
	led_cdev[0].va_sl  = ioremap(GRF_GPIO3C_SL, 4);	 // 
	led_cdev[0].va_p  = ioremap(GRF_GPIO3C_P, 4);	 // 

	alloc_chrdev_region(&devno, 0, DEV_CNT, DEV_NAME);

	led_chrdev_class = class_create(THIS_MODULE, "led_chrdev");

	for (; i < DEV_CNT; i++) {
		cdev_init(&led_cdev[i].dev, &led_chrdev_fops);
		led_cdev[i].dev.owner = THIS_MODULE;

		cur_dev = MKDEV(MAJOR(devno), MINOR(devno) + i);

		cdev_add(&led_cdev[i].dev, cur_dev, 1);

		device_create(led_chrdev_class, NULL, cur_dev, NULL,
			      DEV_NAME "%d", i);
	}

	return 0;
}

module_init(led_chrdev_init);

static __exit void led_chrdev_exit(void)
{
	int i;
	dev_t cur_dev;
	printk("led chrdev exit (GPIO3_C3)\n");
	
	for (i = 0; i < DEV_CNT; i++) {
		iounmap(led_cdev[i].va_dr); 		// 释放模式寄存器虚拟地址
		iounmap(led_cdev[i].va_ddr); 	// 释放输出类型寄存器虚拟地址
		iounmap(led_cdev[i].va_sl); 
		iounmap(led_cdev[i].va_p); 
	}

	for (i = 0; i < DEV_CNT; i++) {
		cur_dev = MKDEV(MAJOR(devno), MINOR(devno) + i);

		device_destroy(led_chrdev_class, cur_dev);

		cdev_del(&led_cdev[i].dev);

	}
	unregister_chrdev_region(devno, DEV_CNT);
	class_destroy(led_chrdev_class);

}

module_exit(led_chrdev_exit);

MODULE_AUTHOR("embedfire");
MODULE_LICENSE("GPL");
