#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#define DEV_NAME            "led_chrdev"
#define DEV_CNT                 (1)

#define GPIO0_BASE (0xfdd60000)
//每组GPIO,有2个寄存器,对应32个引脚，每个寄存器负责16个引脚；
//一个寄存器32位，其中高16位都是使能位，低16位对应16个引脚，每个引脚占用1比特位
#define GPIO0_DR_L (GPIO0_BASE + 0x0000)
#define GPIO0_DR_H (GPIO0_BASE + 0x0004)
#define GPIO0_DDR_L (GPIO0_BASE + 0x0008)
#define GPIO0_DDR_H (GPIO0_BASE + 0x000C)

static dev_t devno;
struct class *led_chrdev_class;

struct led_chrdev {
	struct cdev dev;
	unsigned int __iomem *va_dr; 	// 数据寄存器，设置输出的电压
	unsigned int __iomem *va_ddr; 	// 数据方向寄存器，设置输入或者输出

	unsigned int led_pin; // 偏移
};

static int led_chrdev_open(struct inode *inode, struct file *filp)
{	
	unsigned int val = 0;
	struct led_chrdev *led_cdev = (struct led_chrdev *)container_of(inode->i_cdev, struct led_chrdev,dev);
	filp->private_data = container_of(inode->i_cdev, struct led_chrdev, dev);

	printk("open\n");
	//设置输出模式
	val = ioread32(led_cdev->va_ddr);
	val |= ((unsigned int)0x1 << (led_cdev->led_pin+16));
	val |= ((unsigned int)0X1 << (led_cdev->led_pin));
	iowrite32(val,led_cdev->va_ddr);
	
	//输出高电平
	val = ioread32(led_cdev->va_dr);
	val |= ((unsigned int)0x1 << (led_cdev->led_pin+16));
	val |= ((unsigned int)0x1 << (led_cdev->led_pin));
	iowrite32(val, led_cdev->va_dr);

	return 0;
}

static int led_chrdev_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t led_chrdev_write(struct file *filp, const char __user * buf,
				size_t count, loff_t * ppos)
{
	unsigned long val = 0;
	char ret = 0;

	struct led_chrdev *led_cdev = (struct led_chrdev *)filp->private_data;
	printk("write \n");
	get_user(ret, buf);
	val = ioread32(led_cdev->va_dr);
	printk("val = %lx\n", val);
	if (ret == '0'){
		val |= ((unsigned int)0x1 << (led_cdev->led_pin+16));
		val &= ~((unsigned int)0x01 << (led_cdev->led_pin));   /*设置GPIO引脚输出低电平*/
	}
	else{
		val |= ((unsigned int)0x1 << (led_cdev->led_pin+16));
		val |= ((unsigned int)0x01 << (led_cdev->led_pin));    /*设置GPIO引脚输出高电平*/
	}
	iowrite32(val, led_cdev->va_dr);
	printk("val = %lx\n", val);
	return count;
}

static struct file_operations led_chrdev_fops = {
	.owner = THIS_MODULE,
	.open = led_chrdev_open,
	.release = led_chrdev_release,
	.write = led_chrdev_write,
};

static struct led_chrdev led_cdev[DEV_CNT] = {
	{.led_pin = 7}, 	//偏移，高16引脚,GPIO0_C7
};

static __init int led_chrdev_init(void)
{
	int i = 0;
	dev_t cur_dev;

	printk("led_chrdev init (lubancat2  GPIO0_C7)\n");
	
	led_cdev[0].va_dr   = ioremap(GPIO0_DR_H, 4);	 //
	led_cdev[0].va_ddr  = ioremap(GPIO0_DDR_H, 4);	 // 

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
	printk("led chrdev exit (lubancat2  GPIO0_C7)\n");
	
	for (i = 0; i < DEV_CNT; i++) {
		iounmap(led_cdev[i].va_dr); 		// 释放模式寄存器虚拟地址
		iounmap(led_cdev[i].va_ddr); 	// 释放输出类型寄存器虚拟地址
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
