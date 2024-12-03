/*
*
*   file: menu_dht11.c
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
#include <gpiod.h>

#include "oled.h"
#include "menu.h"
#include "key.h"
#include "config.h"

void ldr_ntc_info_func(void **params)
{
    int ret = 0;
    int ldr_new, ntc_new;
    int ldr_old = 1, ntc_old = 1;
    char ldr_str[20] = {0};
    char ntc_str[20] = {0};

    struct gpiod_chip *gpiochip_ldr;     
    struct gpiod_chip *gpiochip_ntc;          
    struct gpiod_line *gpioline_ldr;          
    struct gpiod_line *gpioline_ntc;  

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

    /* 3、初始化光敏引脚 */
    char ldr_pin_chip[20], ntc_pin_chip[20];
    cJSON *ldrchip = config_get_value("ldr", "ldr_pin_chip");
    cJSON *ldrnum = config_get_value("ldr", "ldr_pin_num");
    cJSON *ntcchip = config_get_value("ntc", "ntc_pin_chip");
    cJSON *ntcnum = config_get_value("ntc", "ntc_pin_num");
    if(ldrchip == NULL || ntcchip == NULL || ldrnum == NULL || ntcnum == NULL)
        goto _exit;

    // 获取gpio控制器
    sprintf(ldr_pin_chip, "/dev/gpiochip%s", ldrchip->valuestring);
    sprintf(ntc_pin_chip, "/dev/gpiochip%s", ntcchip->valuestring);
    gpiochip_ldr = gpiod_chip_open(ldr_pin_chip);  
    gpiochip_ntc = gpiod_chip_open(ntc_pin_chip);
    if(gpiochip_ldr == NULL || gpiochip_ntc == NULL)
    {
        fprintf(stderr, "open gpiochip_ldr or gpiochip_ldr error!\n");
        goto _exit;
    }

    // 获取gpio引脚 
    gpioline_ldr = gpiod_chip_get_line(gpiochip_ldr, ldrnum->valueint);
    gpioline_ntc = gpiod_chip_get_line(gpiochip_ntc, ntcnum->valueint);
    if(gpioline_ldr == NULL || gpioline_ntc == NULL)
    {
        fprintf(stderr, "get gpioline_ldr or gpioline_ntc error!\n");
        goto _exit;
    }

    // 设置ldr ntc脚为输入模式
    ret |= gpiod_line_request_input(gpioline_ldr, "ldr"); 
    ret |= gpiod_line_request_input(gpioline_ntc, "ntc"); 
    if(ret < 0)
    {
        printf("set ldr ntc pin to input mode error!\n");
        goto _exit;
    }

    /* 4、循环显示光敏热敏模块状态 */
    while(key_get_value() != KEY2_PRESSED)
    {
        ldr_new = gpiod_line_get_value(gpioline_ldr);
        ntc_new = gpiod_line_get_value(gpioline_ntc);

        sprintf(ldr_str, "ldr:%s", (ldr_new == 0) ? "overlight" : "normal");                   // 格式化心率字符串
        sprintf(ntc_str, "ntc:%s", (ntc_new == 0) ? "overheating" : "normal");                 // 格式化血氧字符串

        // 输出传感器状态
        //printf("光敏电阻模块DO脚信号:%d, 当前状态:%s\n", ldr, (ldr == 0) ? "过亮" : "正常");
        //printf("热敏电阻模块DO脚信号:%d, 当前状态:%s\n", ntc, (ntc == 0) ? "过热" : "正常");
        //printf("\n");

        if(ldr_old != ldr_new || ntc_old != ntc_new)
        {
            oled_clear_page(2);
            oled_clear_page(3);
            oled_clear_page(4);
            oled_clear_page(5);

            ldr_old = ldr_new;
            ntc_old = ntc_new;
        }

        oled_show_string(2, 2, ldr_str);                // 更新OLED显示心率
        oled_show_string(2, 4, ntc_str);                // 更新OLED显示血氧

        sleep(0.2);
    }

_exit:

    if(gpiochip_ldr != NULL && gpioline_ldr != NULL && gpiochip_ntc != NULL && gpioline_ntc != NULL)
    {
        gpiod_line_release(gpioline_ldr);   
        gpiod_line_release(gpioline_ntc);   
        gpiod_chip_close(gpiochip_ldr); 
        gpiod_chip_close(gpiochip_ntc);
    }

    oled_clear();
}