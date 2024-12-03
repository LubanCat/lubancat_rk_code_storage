/*
*
*   file: menu_max30102.c
*   update: 2024-11-02
*
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "oled.h"
#include "menu.h"
#include "key.h"
#include "config.h"
#include "max30102.h"

#define PPG_DATA_THRESHOLD  100000 	                        // PPG数据检测阈值
#define CACHE_NUMS          150                             // 缓存数

void max30102_info_func(void **params)
{
    int ret;
    uint32_t max30102_data[2], avg_data_IR, avg_data_RED;   // 数据缓存

    float ppg_data_cache_IR[CACHE_NUMS] = {0};              // 红外光缓存区
    float ppg_data_cache_RED[CACHE_NUMS] = {0};             // 红光缓存区
    uint16_t cache_counter = 0;                             // 缓存计数器

    int heartrate = 0;                                      // 心率
    float blood_oxygen = 0;                                 // 血氧饱和度
    char heartrate_str[20] = {0};                           // 存储心率字符串
    char blood_oxygen_str[20] = {0};                        // 存储血氧字符串

    if(params == NULL)
        goto _exit;
    menu_t *menu_Host = (menu_t *)(*params);

    /* 1、清屏 */
    oled_clear();

    /* 2、显示菜单项名称 */
    unsigned char *info_name = menu_Host->info[menu_Host->current_page - 1]->name;
    int len = strlen(info_name);
    int start_x = (128 * 0.5) - (len * 8 * 0.5);
    oled_show_string(start_x, 0, info_name);

    /* 3、max30102初始化 */
    cJSON *max30102_bus = config_get_value("max30102", "bus");
    if(max30102_bus == NULL)
        goto _exit;

    ret = max30102_init(max30102_bus->valueint);
    if(ret < 0)
    {
        fprintf(stderr, "max30102 init err!\n");
        oled_show_string(0, 3, "no sensor...");

        while(key_get_value() != KEY2_PRESSED)
            usleep(1000);

        goto _exit;
    }

    /* 4、循环显示心率血氧数据 */
    while(key_get_value() != KEY2_PRESSED)
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
            oled_show_string(2, 2, "BMP: 00");                   // 更新OLED显示心率
            oled_show_string(2, 4, "SpO2: 00.00%");             // 更新OLED显示血氧
        }

        /* 计算血氧和心率 */
        if(cache_counter >= CACHE_NUMS)                         // 当缓存满时进行计算
        {
            heartrate = max30102_getHeartRate(ppg_data_cache_IR, CACHE_NUMS);                       // 计算心率
            blood_oxygen = max30102_getSpO2(ppg_data_cache_IR, ppg_data_cache_RED, CACHE_NUMS);     // 计算血氧

            sprintf(heartrate_str, "BMP: %d", heartrate);        // 格式化心率字符串
            sprintf(blood_oxygen_str, "SpO2: %.2f%%", blood_oxygen); // 格式化血氧字符串
        
            //printf("心率：%d 次/min\t", heartrate);             // 打印心率
            //printf("血氧：%.2f%%\n", blood_oxygen);             // 打印血氧

            oled_clear_page(2);
            oled_clear_page(3);
            oled_clear_page(4);
            oled_clear_page(5);

            oled_show_string(2, 2, heartrate_str);              // 更新OLED显示心率
            oled_show_string(2, 4, blood_oxygen_str);           // 更新OLED显示血氧

            cache_counter = 0;                                  // 重置缓存计数器
        }
    }

_exit:

    /* 5、max30102反初始化 */
    max30102_exit();

    oled_clear();
}