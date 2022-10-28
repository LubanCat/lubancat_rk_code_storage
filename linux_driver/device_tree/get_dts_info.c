/*
 * Copyright (C) 2022 - All Rights Reserved by
 * EmbedFire LubanCat
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

struct device_node	*led_test_device_node; //设备树节点
struct device_node  *led_device_node; //节点
struct property     *led_property;    //定义属性结构体指针
int size = 0 ;
unsigned int out_values[18];  //保存读取得到 属性值

static const struct of_device_id of_gpio_leds_match[] = {
	{.compatible = "get_dts_info_test"},
	{},
};

static int get_dts_info_probe(struct platform_device *pdev)
{
    int error_status = -1;
	pr_info("%s\n",__func__);

	/*获取DTS属性信息*/
    led_test_device_node = of_find_node_by_path("/get_dts_info_test");
    if(led_test_device_node == NULL)
    {
        printk(KERN_ALERT "get led_device_node failed ! \n");
        return -1;
    }

    /*根据 led_test_device_node 设备节点结构体输出节点的基本信息*/
    printk(KERN_ALERT "name: %s",led_test_device_node->name); //输出节点名
    printk(KERN_ALERT "child name: %s",led_test_device_node->child->name);  //输出子节点的节点名


    /*获取 led_device_node 的子节点*/ 
    led_device_node = of_get_next_child(led_test_device_node,NULL); 
    if(led_device_node == NULL)
    {
        printk(KERN_ALERT "\n get led_device_node failed ! \n");
        return -1;
    }
    printk(KERN_ALERT "name: %s",led_device_node->name); //输出节点名
    printk(KERN_ALERT "parent name: %s",led_device_node->parent->name);  //输出父节点的节点名


    /*获取 led_device_node 节点  的"compatible" 属性 */ 
    led_property = of_find_property(led_device_node,"compatible",&size);
    if(led_property == NULL)
    {
        printk(KERN_ALERT "get led_property failed ! \n");
        return -1;
    }
    printk(KERN_ALERT "size = : %d",size);                      //实际读取得到的长度
    printk(KERN_ALERT "name: %s",led_property->name);   //输出属性名
    printk(KERN_ALERT "length: %d",led_property->length);        //输出属性长度
    printk(KERN_ALERT "value : %s",(char*)led_property->value);  //属性值


    /*获取 reg 地址属性*/
    error_status = of_property_read_u32_array(led_device_node,"reg",out_values, 2);
    if(error_status != 0)
    {
        printk(KERN_ALERT "get out_values failed ! \n");
        return -1;
    }
    printk(KERN_ALERT"0x%08X ", out_values[0]);
    printk(KERN_ALERT"0x%08X ", out_values[1]);

	return 0;
}

static int get_dts_info_remove(struct platform_device *pdev)
{
    pr_info("%s\n",__func__);
	return 0;
}

static struct platform_driver get_dts_info_driver = {
	.probe	= get_dts_info_probe,
    .remove = get_dts_info_remove,
	.driver		= {
		.name	= "get_dts_info_test",
		.of_match_table = of_match_ptr(of_gpio_leds_match),
	},
};

module_platform_driver(get_dts_info_driver);


MODULE_DESCRIPTION("平台驱动：一个简单获取设备树属性的实验");
MODULE_LICENSE("GPL");

