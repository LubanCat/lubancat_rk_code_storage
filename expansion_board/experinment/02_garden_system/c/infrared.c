/*
*
*   file: infrared.c
*   update: 2024-08-14
*   function: 
*   
*/

#include "infrared.h"

/*****************************
 * @brief : 人体红外模块初始化
 * @param : none
 * @return: none
*****************************/
int infrared_init(void)
{
    int ret = 0;

    /* get gpio controller */
    infrared_gpiochip = gpiod_chip_open(GPIOCHIP_DEV);  
    if(infrared_gpiochip == NULL)
    {
        printf("gpiod_chip_open error\n");
        return -1;
    }

    /* get gpio line */
    infrared_line = gpiod_chip_get_line(infrared_gpiochip, GPIONUM_INFRARED);
    if(infrared_line == NULL)
    {
        printf("gpiod_chip_get_line error : 0\n");
        return -1;
    }

    /* set the line direction to input mode */
    ret = gpiod_line_request_input(infrared_line, "infrared_line");   
    if(ret < 0)
    {
        printf("gpiod_line_request_input error : infrared_line\n");
        return -1;
    }

    return 0;
}

/*****************************
 * @brief : 获取人体红外模块输出
 * @param : none
 * @return: 1检测到人体 0未检测到人体
*****************************/
int infrared_get_value(void)
{
    return gpiod_line_get_value(infrared_line);
}

/*****************************
 * @brief : 人体红外模块反初始化
 * @param : none
 * @return: none
*****************************/
void infrared_exit(void)
{
    /* release line */
    gpiod_line_release(infrared_line);
    gpiod_chip_close(infrared_gpiochip);
}