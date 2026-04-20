/*
 * 为了方便实验整理得RK各芯片各GPIO寄存器基地址
 * RK3528：
 *  GPIO0：0xff210000
 *  GPIO1：0xff220000
 *  GPIO2：0xff230000
 *  GPIO3：0xff240000
 * 
 * RK3562：
 *  GPIO0：0xff260000
 *  GPIO1：0xff620000
 *  GPIO2：0xff630000
 *  GPIO3：0xffac0000
 *  GPIO4：0xffad0000
 * 
 * RK3566/RK3568：
 *  GPIO0：0xfdd60000
 *  GPIO1：0xfe740000
 *  GPIO2：0xfe750000
 *  GPIO3：0xfe760000
 *  GPIO4：0xfe770000
 *
 * RK3588S/RK3588：
 *  GPIO0：0xfd8a0000
 *  GPIO1：0xfec20000
 *  GPIO2：0xfec30000
 *  GPIO3：0xfec40000
 *  GPIO4：0xfec50000
 *
 * 为了方便实验整理得各板卡的系统心跳灯
 * RK3528：
 * LubanCat-Q1、LubanCat-Q1IO：(GPIO4_B5)
 * RK3562：
 * LubanCat-1HS：(GPIO3_A7)
 * RK3566：
 * LubanCat-0、LubanCat-1、LubanCat-1n：(GPIO0_C5)
 * LubanCat-1H：(GPIO0_C7)
 * LubanCat-1IO：(GPIO0_C0)
 * RK3568：
 * LubanCat-2、LubanCat-2-V1、LubanCat-2-V2、LubanCat-2H、LubanCat-2N、LubanCat-2N-V2、LubanCat-2N-V3：(GPIO0_C7)
 * LubanCat-2-V3：(GPIO1_A4)
 * LubanCat-2IO：(GPIO4_D2)
 * RK3576：
 * LubanCat-3、LubanCat-3-V2、LubanCat-3IO：(GPIO3_C5)
 * RK3588S：
 * LubanCat-4、LubanCat-4-V1：(GPIO4_B5)
 * LubanCat-4IO：(GPIO1_C6)
 * RK3588：
 * LubanCat-5、LubanCat-5-V2：(GPIO0_D3)
 * LubanCat-5IO：(GPIO1_C6)
 *
 * 本实验默认以RK3568的GPIO0_C7为例，如果需要修改为其他引脚，需修改
 * 1、GPIO寄存器基地址，RK3568的GPIO0基地址为0xFDD60000
 *    #define GPIO_BASE (0xFDD60000)   
 * 2、引脚偏移，A、B端口的引脚属于低16个引脚，C、D端口的引脚属于高16个引脚，GPIO0_C7中的C7是高16个引脚中索引号为7的引脚。
 *    unsigned int pdev_led_hwinfo[1] = { 7 };
 * 3、数据寄存器和数据方向寄存器映射
 *    A、B端口使用GPIO_DR_L、GPIO_DDR_L
 *    C、D端口使用GPIO_DR_H、GPIO_DDR_H
 *    GPIO0_C7的端口为C，使用GPIO_DR_H、GPIO_DDR_H
 *    [0] = DEFINE_RES_MEM(GPIO_DR_H, 4),
 *    [1] = DEFINE_RES_MEM(GPIO_DDR_H, 4),
 *
 * 如果修改为RK3588S的GPIO4_B5
 *    #define GPIO_BASE (0xFEC50000)               // GPIO4的基地址
 *
 *    unsigned int pdev_led_hwinfo[1] = { 13 };    // 偏移，GPIO4_B5，B端口属于低16个引脚，B5为8+5位，即偏移13
 * 
 *    [0] = DEFINE_RES_MEM(GPIO_DR_L, 4),          // B端口使用GPIO_DR_L、GPIO_DDR_L
 *    [1] = DEFINE_RES_MEM(GPIO_DDR_L, 4),
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

/* 每组 GPIO 有 2 个寄存器，对应 32 个引脚，每个寄存器负责 16 个引脚；
 * 一个寄存器 32 位，其中高 16 位都是使能位，低 16 位对应 16 个引脚，每个引脚占用 1 比特位
 * 定义 GPIO 寄存器的基地址 */
#define GPIO_BASE (0xfdd60000)
/* 定义 GPIO 数据寄存器低 16 位的地址 */
#define GPIO_DR_L (GPIO_BASE + 0x0000)
/* 定义 GPIO 数据寄存器高 16 位的地址 */
#define GPIO_DR_H (GPIO_BASE + 0x0004)
/* 定义 GPIO 数据方向寄存器低 16 位的地址 */
#define GPIO_DDR_L (GPIO_BASE + 0x0008)
/* 定义 GPIO 数据方向寄存器高 16 位的地址 */
#define GPIO_DDR_H (GPIO_BASE + 0x000C)

/* 定义平台设备的资源数组，包含 GPIO 数据寄存器高 16 位和 GPIO 数据方向寄存器高 16 位的资源 */
static struct resource pdev_led_resource[] = {
    /* 定义 GPIO 数据寄存器高 16 位的内存资源，起始地址为 GPIO_DR_H，长度为 4 字节 */
    [0] = DEFINE_RES_MEM(GPIO_DR_H, 4),
    /* 定义 GPIO 数据方向寄存器高 16 位的内存资源，起始地址为 GPIO_DDR_H，长度为 4 字节 */
    [1] = DEFINE_RES_MEM(GPIO_DDR_H, 4),
};

/* 定义 LED 硬件信息数组，包含 LED 引脚的偏移量 */
unsigned int pdev_led_hwinfo[1] = { 7 };  /* 偏移，高 16 引脚，GPIO0_C7 */

/* 平台设备释放函数，当平台设备被移除时调用 */
static void pdev_led_release(struct device *dev)
{
    /* 打印平台设备释放信息 */
    printk("pdev release\r\n");
}

/* 定义平台设备结构体 */
static struct platform_device pdev_led = {
    /* 平台设备的名称，用于和平台驱动进行匹配 */
    .name = "pdev_led",
    /* 平台设备的 ID */
    .id = 0,
    /* 平台设备的资源数量，通过 ARRAY_SIZE 宏计算 pdev_led_resource 数组的大小 */
    .num_resources = ARRAY_SIZE(pdev_led_resource),
    /* 指向平台设备的资源数组 */
    .resource = pdev_led_resource,
    /* 平台设备的设备结构体 */
    .dev = {
        /* 设备释放函数指针，指向 pdev_led_release 函数 */
        .release = pdev_led_release,
        /* 平台设备的私有数据，指向 pdev_led_hwinfo 数组 */
        .platform_data = pdev_led_hwinfo,
    },
};

/* 模块初始化函数，在模块加载时调用 */
static __init int pdev_led_init(void)
{
    /* 打印平台设备初始化信息 */
    printk("pdev init\n");
    /* 注册平台设备，将 pdev_led 平台设备注册到内核中 */
    platform_device_register(&pdev_led);
    return 0;
}

/* 模块退出函数，在模块卸载时调用 */
static __exit void pdev_led_exit(void)
{
    /* 打印平台设备退出信息 */
    printk("pdev exit\n");
    /* 注销平台设备，将 pdev_led 平台设备从内核中注销 */
    platform_device_unregister(&pdev_led);
}

module_init(pdev_led_init);
module_exit(pdev_led_exit);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("pdev_led module");
MODULE_LICENSE("GPL");