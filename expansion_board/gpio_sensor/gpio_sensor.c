/*
*
*   file: gpio_sensor.c
*   update: 2024-10-16
*   usage: 
*       sudo gcc -o gpio_sensor gpio_sensor.c -lgpiod
*       sudo ./gpio_sensor
*
*/

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h> 

// 定义全局变量，用于操作GPIO芯片和引脚
struct gpiod_chip *gpiochip6;        
struct gpiod_line *gpioline8;          
struct gpiod_line *gpioline9;  
struct gpiod_line *gpioline10;

/*****************************
 * @brief : 处理SIGINT信号的处理函数
 * @param : sig_num - 信号编号
 * @return: none
*****************************/
void sigint_handler(int sig_num) 
{    
    // 释放GPIO引脚
    gpiod_line_release(gpioline8);
    gpiod_line_release(gpioline9);
    gpiod_line_release(gpioline10);

    // 关闭GPIO芯片
    gpiod_chip_close(gpiochip6);

    exit(0);  
}

/*****************************
 * @brief : 传感器模块初始化
 * @param : none
 * @return: none
*****************************/
int modules_init()
{
    int ret;

    // 打开GPIO芯片
    gpiochip6 = gpiod_chip_open("/dev/gpiochip6");  
    if(gpiochip6 == NULL)
    {
        printf("gpiod_chip_open error\n");
        return -1;
    }

    // 获取GPIO引脚8
    gpioline8 = gpiod_chip_get_line(gpiochip6, 8);
    if(gpioline8 == NULL)
    {
        printf("gpiod_chip_get_line error : 0\n");
        return -1;
    }
    // 获取GPIO引脚9
    gpioline9 = gpiod_chip_get_line(gpiochip6, 9);
    if(gpioline9 == NULL)
    {
        printf("gpiod_chip_get_line error : 0\n");
        return -1;
    }
    // 获取GPIO引脚10
    gpioline10 = gpiod_chip_get_line(gpiochip6, 10);
    if(gpioline10 == NULL)
    {
        printf("gpiod_chip_get_line error : 0\n");
        return -1;
    }

    // 请求将引脚8、9和10设置为输入模式
    ret |= gpiod_line_request_input(gpioline8, "gpioline8"); 
    ret |= gpiod_line_request_input(gpioline9, "gpioline9"); 
    ret |= gpiod_line_request_input(gpioline10, "gpioline10"); 
    if(ret < 0)
    {
        printf("gpiod_line_request_input error\n");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    int ret;
    int ldr, thermistor, flame;             // 用于存储传感器状态的变量

    // 注册SIGINT信号的处理函数（Ctrl + C）
    signal(SIGINT, sigint_handler);

    // 初始化模块
    ret = modules_init();
    if(ret == -1)
        return ret;

    while(1)
    {  
        // 获取三个传感器的状态
        ldr = gpiod_line_get_value(gpioline8);
        thermistor = gpiod_line_get_value(gpioline9);
        flame = gpiod_line_get_value(gpioline10);

        // 输出传感器状态
        printf("光敏电阻模块DO脚信号:%d, 当前状态:%s\n", ldr, (ldr == 0) ? "过亮" : "正常");
        printf("热敏电阻模块DO脚信号:%d, 当前状态:%s\n", thermistor, (thermistor == 0) ? "过热" : "正常");
        printf("火焰检测模块DO脚信号:%d, 当前状态:%s\n", flame, (flame == 0) ? "有火焰" : "正常");
        printf("\n");

        sleep(1);
    }

    return 0;
}