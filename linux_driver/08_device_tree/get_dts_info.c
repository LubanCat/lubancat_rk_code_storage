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

/* 
 * 定义设备节点指针，用于指向 DTS 中的设备节点。
 * led_test_device_node 指向根节点下的 "/get_dts_info_test" 节点。
 * led_device_node 指向 led_test_device_node 的子节点。
 */
struct device_node	*led_test_device_node;
struct device_node  *led_device_node;

/* 
 * 定义属性指针，用于指向设备节点的属性。
 * 该指针将用于获取和操作设备节点的特定属性。
 */
struct property     *led_property;

/* 
 * 用于存储属性长度的变量。
 * 在读取设备节点属性时，会将属性的实际长度存储在此变量中。
 */
int size = 0 ;

/* 
 * 用于存储从 DTS 中读取的 32 位无符号整数数组。
 * 主要用于存储从 "reg" 属性中读取的地址信息。
 */
unsigned int out_values[18];

/* 
 * get_dts_info_probe 函数是平台驱动的探测函数。
 * 功能：从设备树中获取指定节点的信息，包括节点名称、属性信息和地址信息等。
 * 参数：
 *   pdev：指向平台设备结构体的指针，包含匹配到的设备信息。
 */
static int get_dts_info_probe(struct platform_device *pdev)
{
    /* 初始化错误状态变量，用于记录操作的结果 */
    int error_status = -1;

    /* 打印当前函数名，方便调试时确认函数调用 */
    pr_info("%s\n",__func__);

    /* 获取 DTS 属性信息 */
    /* 在设备树中查找路径为 "/get_dts_info_test" 的设备节点 */
    led_test_device_node = of_find_node_by_path("/get_dts_info_test");
    //led_test_device_node = pdev->dev.of_node;
    if(led_test_device_node == NULL)
    {
        /* 如果查找失败，打印错误信息并返回 -1 */
        printk("get led_device_node failed ! \n");
        return -1;
    }

    /* 根据 led_test_device_node 设备节点结构体输出节点的基本信息 */
    /* 输出设备节点的名称 */
    printk("name: %s",led_test_device_node->name); 
    /* 输出设备节点的子节点的名称 */
    printk("child name: %s",led_test_device_node->child->name);  

    /* 获取 led_test_device_node 的下一个子节点 */
    led_device_node = of_get_next_child(led_test_device_node,NULL); 
    if(led_device_node == NULL)
    {
        /* 如果获取子节点失败，打印错误信息并返回 -1 */
        printk("\n get led_device_node failed ! \n");
        return -1;
    }

    /* 输出子节点的名称 */
    printk("name: %s",led_device_node->name); 
    /* 输出子节点的父节点的名称 */
    printk("parent name: %s",led_device_node->parent->name);  

    /* 获取 led_device_node 节点的 "compatible" 属性 */ 
    /* 在 led_device_node 节点中查找 "compatible" 属性，并获取其长度存储到 size 中 */
    led_property = of_find_property(led_device_node,"compatible",&size);
    if(led_property == NULL)
    {
        /* 如果查找属性失败，打印错误信息并返回 -1 */
        printk("get led_property failed ! \n");
        return -1;
    }

    /* 输出属性的实际读取长度 */
    printk("size = : %d",size);                      
    /* 输出属性的名称 */
    printk("name: %s",led_property->name);   
    /* 输出属性的长度 */
    printk("length: %d",led_property->length);        
    /* 输出属性的值（假设值是可打印的字符串） */
    printk("value : %s",(char*)led_property->value);  

    /* 获取 reg 地址属性 */
    /* 从 led_device_node 节点的 "reg" 属性中读取 32 位无符号整数数组，最多读取 2 个 */
    error_status = of_property_read_u32_array(led_device_node,"reg",out_values, 2);
    if(error_status != 0)
    {
        /* 如果读取失败，打印错误信息并返回 -1 */
        printk("get out_values failed ! \n");
        return -1;
    }

    /* 输出读取到的第一个 32 位无符号整数值 */
    printk("0x%08X ", out_values[0]);
    /* 输出读取到的第二个 32 位无符号整数值 */
    printk("0x%08X ", out_values[1]);

    return 0;
}

/* 平台驱动的移除函数 */
static int get_dts_info_remove(struct platform_device *pdev)
{
    printk("get_dts_info remove\r\n");
    return 0;
}

/* 
 * 定义设备树匹配表，用于与 DTS 中的 compatible 属性进行匹配。
 * 当设备树中的节点的 compatible 属性值为 "get_dts_info_test" 时，认为匹配成功。
 */
static const struct of_device_id of_gpio_leds_match[] = {
    {.compatible = "get_dts_info_test"},
    {}
};

/* 
 * 定义平台驱动结构体，包含探测、移除函数以及驱动的相关信息。
 * 该结构体将作为平台驱动的核心，与内核中的平台总线进行交互。
 */
static struct platform_driver get_dts_info_driver = {
    /* 探测函数指针，指向 get_dts_info_probe 函数 */
   .probe   = get_dts_info_probe,
    /* 移除函数指针，指向 get_dts_info_remove 函数 */
   .remove = get_dts_info_remove,
   .driver        = {
        /* 驱动的名称，用于标识该驱动 */
       .name    = "get_dts_info_test",
        /* 设备树匹配表指针，指向 of_gpio_leds_match */
       .of_match_table = of_match_ptr(of_gpio_leds_match),
    }
};

/* 注册平台驱动，该宏将 get_dts_info_driver 结构体注册到内核的平台总线中，使驱动可以与设备进行匹配 */
module_platform_driver(get_dts_info_driver);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("get_dts_info module");
MODULE_LICENSE("GPL");