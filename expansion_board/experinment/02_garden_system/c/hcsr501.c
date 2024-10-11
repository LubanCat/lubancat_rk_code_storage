/*
*
*   file: hcsr501.c
*   update: 2024-08-14
*   function: 
*   
*/

#include "hcsr501.h"

static struct gpiod_chip *hcsr501_gpiochip;        
static struct gpiod_line *hcsr501_line; 

/*****************************
 * @brief : 人体红外模块初始化
 * @param : none
 * @return: none
*****************************/
int hcsr501_init(const char *gpiochip, unsigned int gpionum)
{
    int ret = 0;

    if(gpiochip == NULL)
        return -1;

    /* get gpio controller */
    hcsr501_gpiochip = gpiod_chip_open(gpiochip);  
    if(hcsr501_gpiochip == NULL)
    {
        printf("gpiod_chip_open error\n");
        return -1;
    }

    /* get gpio line */
    hcsr501_line = gpiod_chip_get_line(hcsr501_gpiochip, gpionum);
    if(hcsr501_line == NULL)
    {
        printf("gpiod_chip_get_line error : 0\n");
        return -1;
    }

    /* set the line direction to input mode */
    ret = gpiod_line_request_input(hcsr501_line, "hcsr501_line");   
    if(ret < 0)
    {
        printf("gpiod_line_request_input error : hcsr501_line\n");
        return -1;
    }

    return 0;
}

/*****************************
 * @brief : 获取人体红外模块输出
 * @param : none
 * @return: 1检测到人体 0未检测到人体
*****************************/
int hcsr501_get_value(void)
{   
    if(hcsr501_line == NULL)
        return -1;

    return gpiod_line_get_value(hcsr501_line);
}

/*****************************
 * @brief : 人体红外模块反初始化
 * @param : none
 * @return: none
*****************************/
void hcsr501_exit(void)
{   
    if(hcsr501_line == NULL || hcsr501_gpiochip == NULL)
        return;

    /* release line */
    gpiod_line_release(hcsr501_line);
    gpiod_chip_close(hcsr501_gpiochip);
}