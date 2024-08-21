/*
*
*   file: led.c
*   update: 2024-08-09
*   function: 适用于野火鲁班猫扩展板
*
*/

#include "led.h"

/*****************************
 * @brief : led初始化
 * @param : none
 * @return: none
*****************************/
int led_init(void)
{
    int ret = 0;

    /* get gpio controller */
    led_gpiochip = gpiod_chip_open(GPIOCHIP_DEV);  
    if(led_gpiochip == NULL)
    {
        printf("gpiod_chip_open error\n");
        return -1;
    }

    /* get gpio line */
    /* red led */
    r_led_line = gpiod_chip_get_line(led_gpiochip, GPIONUM_R_LED);
    if(r_led_line == NULL)
    {
        printf("gpiod_chip_get_line error : 0\n");
        return -1;
    }

    /* green led */
    g_led_line = gpiod_chip_get_line(led_gpiochip, GPIONUM_G_LED);
    if(g_led_line == NULL)
    {
        printf("gpiod_chip_get_line error : 1\n");
        return -1;
    }

    /* blue led */
    b_led_line = gpiod_chip_get_line(led_gpiochip, GPIONUM_B_LED);
    if(b_led_line == NULL)
    {
        printf("gpiod_chip_get_line error : 2\n");
        return -1;
    }

    /* set the line direction to output mode, and the initial level is high */
    /* red led */
    ret = gpiod_line_request_output(r_led_line, "r_led_line", 1);   
    if(ret < 0)
    {
        printf("gpiod_line_request_output error : r_led_line\n");
        return -1;
    }

    /* green led */
    ret = gpiod_line_request_output(g_led_line, "g_led_line", 1);   
    if(ret < 0)
    {
        printf("gpiod_line_request_output error : g_led_line\n");
        return -1;
    }

    /* blue led */
    ret = gpiod_line_request_output(b_led_line, "b_led_line", 1);   
    if(ret < 0)
    {
        printf("gpiod_line_request_output error : b_led_line\n");
        return -1;
    }

    return 0;
}

/*****************************
 * @brief : led使能
 * @param : line，要使能的led（r_led_line：红灯，g_led_line：绿灯，b_led_line：蓝灯）
 * @return: none
*****************************/
void led_on(struct gpiod_line *line)
{
    if(line == NULL)
        return;

    gpiod_line_set_value(line, 0);
}

/*****************************
 * @brief : led关闭
 * @param : line，要关闭的led（r_led_line：红灯，g_led_line：绿灯，b_led_line：蓝灯）
 * @return: none
*****************************/
void led_off(struct gpiod_line *line)
{
    if(line == NULL)
        return;

    gpiod_line_set_value(line, 1);
}

/*****************************
 * @brief : led反初始化
 * @param : none
 * @return: none
*****************************/
void led_release(void)
{
    /* release line */
    gpiod_line_release(r_led_line);
    gpiod_line_release(g_led_line);
    gpiod_line_release(b_led_line);

    gpiod_chip_close(led_gpiochip);
}