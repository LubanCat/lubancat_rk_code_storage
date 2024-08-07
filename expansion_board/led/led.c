/*
*
*   file: led.c
*   update: 2024-08-07
*   usage: 
*       sudo gcc -o led led.c -lgpiod
*       sudo ./led
*
*/

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h> 

#define GPIONUM_R_LED  (0)
#define GPIONUM_G_LED  (1)
#define GPIONUM_B_LED  (2)

struct gpiod_chip *led_gpiochip;        
struct gpiod_line *r_led_line;          
struct gpiod_line *g_led_line;       
struct gpiod_line *b_led_line;

static void sigint_handler(int sig_num) 
{    
    /* led off */
    gpiod_line_set_value(r_led_line, 1);
    gpiod_line_set_value(g_led_line, 1);
    gpiod_line_set_value(b_led_line, 1);

    /* release line */
    gpiod_line_release(r_led_line);
    gpiod_line_release(g_led_line);
    gpiod_line_release(b_led_line);

    gpiod_chip_close(led_gpiochip);

    exit(0);  
}

int main(int argc, char **argv)
{
    int ret;

    /* register exit signal ( Ctrl + c ) */
    signal(SIGINT, sigint_handler);

    /* get gpio controller */
    led_gpiochip = gpiod_chip_open("/dev/gpiochip6");  
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

    while(1)
    {   
        /* led off */
        gpiod_line_set_value(r_led_line, 1);
        gpiod_line_set_value(g_led_line, 1);
        gpiod_line_set_value(b_led_line, 1);

        /* delay 0.5s */
        usleep(500000);

        /* led on */
        gpiod_line_set_value(r_led_line, 0);
        gpiod_line_set_value(g_led_line, 0);
        gpiod_line_set_value(b_led_line, 0);

        usleep(500000);
    }

    return 0;
}