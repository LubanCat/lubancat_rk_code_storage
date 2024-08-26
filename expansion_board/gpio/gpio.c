/*
*
*   file: gpio.c
*   update: 2024-08-13
*   usage: 
*       sudo gcc -o gpio gpio.c -lgpiod
*       sudo ./gpio
*
*/

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h> 

#define GPIOCHIP_6      "/dev/gpiochip6"

#define GPIONUM_8       (8)
#define GPIONUM_9       (9)

struct gpiod_chip *gpiochip6;        
struct gpiod_line *gpioline8;          
struct gpiod_line *gpioline9;  

static void sigint_handler(int sig_num) 
{    
    gpiod_line_release(gpioline8);
    gpiod_line_release(gpioline9);

    gpiod_chip_close(gpiochip6);

    exit(0);  
}

int main(int argc, char **argv)
{
    int ret;

    /* register exit signal ( Ctrl + c ) */
    signal(SIGINT, sigint_handler);

    /* get gpio controller */
    gpiochip6 = gpiod_chip_open(GPIOCHIP_6);  
    if(gpiochip6 == NULL)
    {
        printf("gpiod_chip_open error\n");
        return -1;
    }

    /* get gpio line */
    gpioline8 = gpiod_chip_get_line(gpiochip6, GPIONUM_8);
    if(gpioline8 == NULL)
    {
        printf("gpiod_chip_get_line error : 0\n");
        return -1;
    }

    gpioline9 = gpiod_chip_get_line(gpiochip6, GPIONUM_9);
    if(gpioline9 == NULL)
    {
        printf("gpiod_chip_get_line error : 0\n");
        return -1;
    }

    /* 设置gpio6_8为输出模式，初始电平为高 */
    ret = gpiod_line_request_output(gpioline8, "gpioline8", 1);   
    if(ret < 0)
    {
        printf("gpiod_line_request_output error : gpioline8\n");
        return -1;
    }

    /* 设置gpio6_9为输入模式*/
    ret = gpiod_line_request_input(gpioline9, "gpioline9");   
    if(ret < 0)
    {
        printf("gpiod_line_request_input error : gpioline9\n");
        return -1;
    }

    while(1)
    {   
        int bit;

        gpiod_line_set_value(gpioline8, 1);
        bit = gpiod_line_get_value(gpioline9);
        printf("%d\n", bit);

        sleep(1);

        gpiod_line_set_value(gpioline8, 0);
        bit = gpiod_line_get_value(gpioline9);
        printf("%d\n", bit);

        sleep(1);
    }

    return 0;
}