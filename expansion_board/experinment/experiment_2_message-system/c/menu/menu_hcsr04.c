/*
*
*   file: menu_hcsr04.c
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
#include "hcsr04.h"

void hcsr04_info_func(void **params)
{
    int ret;
    float dist = 0;
    char dist_str[20] = {0};
    int buzzer_delayweight = 1;                     // 蜂鸣器鸣响的延迟权重
    struct gpiod_chip *buzzer_gpiochip;             //
    struct gpiod_line *buzzer_line;                 //

    if(params == NULL)
        goto _exit;
    menu_t *menu_Host = (menu_t *)(*params);
    
    /* 1、清屏 */
    oled_clear();

    /* 2、显示菜单项名称 */
    char *info_name = menu_Host->info[menu_Host->current_page - 1]->name;
    int len = strlen(info_name);
    int start_x = (128 * 0.5) - (len * 8 * 0.5);
    oled_show_string(start_x, 0, info_name);

    /* 3、初始化蜂鸣器 */
    char buzzer_pin_chip[20];
    cJSON *buzzerchip = config_get_value("buzzer", "pin_chip");
    cJSON *buzzernum = config_get_value("buzzer", "pin_num");
    if(buzzerchip == NULL || buzzernum == NULL)
        goto _exit;

    // 获取gpio控制器
    sprintf(buzzer_pin_chip, "/dev/gpiochip%s", buzzerchip->valuestring);
    buzzer_gpiochip = gpiod_chip_open(buzzer_pin_chip);  
    if(buzzer_gpiochip == NULL)
    {
        fprintf(stderr, "open buzzer-gpiochip error!\n");
        goto _exit;
    }

    // 获取gpio引脚 
    buzzer_line = gpiod_chip_get_line(buzzer_gpiochip, buzzernum->valueint);
    if(buzzer_line == NULL)
    {
        fprintf(stderr, "get buzzer-line error!\n");
        goto _exit;
    }

    // 设置引脚为输出模式，初始电平为低电平
    ret = gpiod_line_request_output(buzzer_line, "buzzer_line", 0);   
    if(ret < 0)
    {
        fprintf(stderr, "set buzzer-line to output mode error!\n");
        goto _exit;
    }

    /* 4、超声波初始化 */
    cJSON *trig_pin_chip = config_get_value("hcsr04", "trig_pin_chip");
    cJSON *echo_pin_chip = config_get_value("hcsr04", "echo_pin_chip");
    cJSON *trig_pin_num = config_get_value("hcsr04", "trig_pin_num");
    cJSON *echo_pin_num = config_get_value("hcsr04", "echo_pin_num");
    if(trig_pin_chip == NULL || echo_pin_chip == NULL || trig_pin_num == NULL || echo_pin_num == NULL)
        goto _exit;

    char trig[20], echo[20];
    sprintf(trig, "/dev/gpiochip%s", trig_pin_chip->valuestring);
    sprintf(echo, "/dev/gpiochip%s", echo_pin_chip->valuestring);

    ret = hc_sr04_init(trig, trig_pin_num->valueint, echo, echo_pin_num->valueint);
    if(ret == -1)
    {
        fprintf(stderr, "hc_sr04 init error!\n");

        oled_show_string(0, 3, "no sensor...");

        while(key_get_value() != KEY2_PRESSED)
            usleep(1000);

        goto _exit;
    }

    /* 5、循序显示超声波测距距离，同时模拟倒车雷达 */
    while(key_get_value() != KEY2_PRESSED)
    {
        // 获取超声波测距距离
        dist = hc_sr04_get_distance();
        if(dist < 10)
            sprintf(dist_str, "000%.2f cm", dist);
        else
            sprintf(dist_str, "00%.2f cm", dist);

        // 在检测距离范围内，蜂鸣器鸣响
        if (dist >= 2 && dist <= 15)
        {
            gpiod_line_set_value(buzzer_line, 1);
            usleep(dist*0.01*buzzer_delayweight*1000000);
            gpiod_line_set_value(buzzer_line, 0);
            usleep(dist*0.01*buzzer_delayweight*1000000);
        }
        else
            gpiod_line_set_value(buzzer_line, 0);

        // OLED上显示距离
        len = strlen(dist_str);
        start_x = (128 * 0.5) - (len * 8 * 0.5);
        oled_show_string(start_x, 3, dist_str);

        usleep(100);
    }

_exit:

    /* 6、超声波模块反初始化 */
    hc_sr04_exit();

    /* 7、蜂鸣器反初始化 */
    if(buzzer_gpiochip != NULL && buzzer_line != NULL)
    {
        gpiod_line_set_value(buzzer_line, 0);         
        gpiod_line_release(buzzer_line);     
        gpiod_chip_close(buzzer_gpiochip); 
    }

    oled_clear();
}