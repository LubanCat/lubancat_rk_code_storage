/*
*
*   file: main.c
*   update: 2024-10-23
*   usage: 
*       make
*       sudo ./main
*
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <stdint.h>
#include <signal.h> 
#include <linux/input.h>
#include <math.h>
#include <pthread.h>
#include <time.h>

#include "oled.h"
#include "max30102.h"

#define PPG_DATA_THRESHOLD  100000 	        // PPG数据检测阈值
#define CACHE_NUMS          150             // 缓存数

int oled_init_flag = 1;                     // OLED初始化标志

void sigint_handler(int sig_num) 
{    
    /* oled清屏 */
    if(oled_init_flag != 0)
        oled_clear();

    exit(0);  
}

int main(int argc, char **argv)
{
    int ret = 0;
    uint32_t max30102_data[2], avg_data_IR, avg_data_RED;   // 数据缓存

    float ppg_data_cache_IR[CACHE_NUMS] = {0};              // 红外光缓存区
    float ppg_data_cache_RED[CACHE_NUMS] = {0};             // 红光缓存区
    uint16_t cache_counter = 0;                             // 缓存计数器

    int heartrate = 0;                                      // 心率
    float blood_oxygen = 0;                                 // 血氧饱和度
    char heartrate_str[20] = {0};                           // 存储心率字符串
    char blood_oxygen_str[20] = {0};                        // 存储血氧字符串

    /* oled初始化 */
    ret = oled_init(3);
    if(ret == -1)
    {
        printf("oled init err!\n");
        oled_init_flag = 0;
    }

    /* max30102初始化 */
    ret = max30102_init(5);
    if(ret == -1)
    {
        printf("max30102 init err!\n");
        return -1;
    }

    signal(SIGINT, sigint_handler);                         // 注册信号处理函数

    sprintf(heartrate_str, "%d BMP", heartrate);            // 格式化心率字符串
    sprintf(blood_oxygen_str, "%.2f %%", blood_oxygen);     // 格式化血氧字符串
    oled_show_string(2, 2, heartrate_str);                  // 显示心率
    oled_show_string(2, 4, blood_oxygen_str);               // 显示血氧

    while(1)
    {   
        /* 读FIFO */
        max30102_read_fifo(max30102_data);
        max30102_average_filter(max30102_data[0], max30102_data[1], &avg_data_IR, &avg_data_RED);
        //printf("raw data : %d , %d\n", max30102_data[0], max30102_data[1]);
        //printf("filter data : %d , %d\n", avg_data_IR, avg_data_RED);

        /* 收集数据 */
        if(avg_data_IR > PPG_DATA_THRESHOLD && avg_data_RED > PPG_DATA_THRESHOLD)
        {
            ppg_data_cache_IR[cache_counter] = avg_data_IR;     // 缓存红外光数据
            ppg_data_cache_RED[cache_counter] = avg_data_RED;   // 缓存红光数据
            cache_counter++;                                    // 更新缓存计数器
        }
        else
        {
            printf("未检测到手指！\n");
        }

        /* 计算血氧和心率 */
        if(cache_counter >= CACHE_NUMS)                         // 当缓存满时进行计算
        {
            heartrate = max30102_getHeartRate(ppg_data_cache_IR, CACHE_NUMS);                       // 计算心率
            blood_oxygen = max30102_getSpO2(ppg_data_cache_IR, ppg_data_cache_RED, CACHE_NUMS);     // 计算血氧
            sprintf(heartrate_str, "%d BMP", heartrate);        // 格式化心率字符串
            sprintf(blood_oxygen_str, "%.2f %%", blood_oxygen); // 格式化血氧字符串

            printf("心率：%d 次/min\t", heartrate);             // 打印心率
            printf("血氧：%.2f%%\n", blood_oxygen);             // 打印血氧

            oled_show_string(2, 2, heartrate_str);              // 更新OLED显示心率
            oled_show_string(2, 4, blood_oxygen_str);           // 更新OLED显示血氧

            cache_counter = 0;                                  // 重置缓存计数器
        }

        //usleep(120000);
    }

    return 0;
}