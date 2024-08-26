/*
*
*   file: buzzer.c
*   update: 2024-08-09
*   function: 适用于野火鲁班猫扩展板
*   
*/

#include "buzzer.h"

/*****************************
 * @brief : 蜂鸣器初始化
 * @param : none
 * @return: none
*****************************/
int buzzer_init(void)
{
    int ret = 0;

    /* get gpio controller */
    buzzer_gpiochip = gpiod_chip_open(GPIOCHIP_DEV);  
    if(buzzer_gpiochip == NULL)
    {
        printf("gpiod_chip_open error\n");
        return -1;
    }

    /* get gpio line */
    buzzer_line = gpiod_chip_get_line(buzzer_gpiochip, GPIONUM_BUZZER);
    if(buzzer_line == NULL)
    {
        printf("gpiod_chip_get_line error : 0\n");
        return -1;
    }

    /* set the line direction to output mode, and the initial level is low */
    ret = gpiod_line_request_output(buzzer_line, "buzzer_line", 0);   
    if(ret < 0)
    {
        printf("gpiod_line_request_output error : buzzer_line\n");
        return -1;
    }

    return 0;
}

/*****************************
 * @brief : 蜂鸣器使能
 * @param : none
 * @return: none
*****************************/
void buzzer_on(void)
{
    gpiod_line_set_value(buzzer_line, 1);
}

/*****************************
 * @brief : 蜂鸣器关闭
 * @param : none
 * @return: none
*****************************/
void buzzer_off(void)
{
    gpiod_line_set_value(buzzer_line, 0);
}

/*****************************
 * @brief : 蜂鸣器反初始化
 * @param : none
 * @return: none
*****************************/
void buzzer_release(void)
{
    /* release line */
    gpiod_line_release(buzzer_line);
    gpiod_chip_close(buzzer_gpiochip);
}