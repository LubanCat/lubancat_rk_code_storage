/*
*
*   file: hc_sr04.c
*   update: 2024-08-27
*   usage: 
*       sudo gcc -o hc_sr04 hc_sr04.c -lgpiod
*       sudo ./hc_sr04.c
*
*/

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h> 
#include <time.h>

#define GPIOCHIP_TRIG           "/dev/gpiochip3"
#define GPIOCHIP_ECHO           "/dev/gpiochip3"

/*
* A-D : 0-3
* number = group * 8 + x
* e.g. : B0 = 1 * 8 + 0 = 8
*	     C4 = 2 * 8 + 4 = 20 
*
*/
#define GPIONUM_TRIC            (17)
#define GPIONUM_ECHO            (14)

struct gpiod_chip *gpiochip_trig;      
struct gpiod_chip *gpiochip_echo;

struct gpiod_line *gpioline_trig;          
struct gpiod_line *gpioline_echo;

static void sigint_handler(int sig_num) 
{    
    gpiod_line_release(gpioline_trig);
    gpiod_line_release(gpioline_echo);

    gpiod_chip_close(gpiochip_trig);
    gpiod_chip_close(gpiochip_echo);

    exit(0);  
}

int gpio_init(void)
{
    int ret;

    /* get gpio controller */
    gpiochip_trig = gpiod_chip_open(GPIOCHIP_TRIG);  
    if(gpiochip_trig == NULL)
    {
        printf("gpiod_chip_open error\n");
        return -1;
    }

    gpiochip_echo = gpiod_chip_open(GPIOCHIP_ECHO);  
    if(gpiochip_echo == NULL)
    {
        printf("gpiod_chip_open error\n");
        return -1;
    }

    /* get gpio line */
    gpioline_trig = gpiod_chip_get_line(gpiochip_trig, GPIONUM_TRIC);
    if(gpioline_trig == NULL)
    {
        printf("gpiod_chip_get_line error\n");
        return -1;
    }

    gpioline_echo = gpiod_chip_get_line(gpiochip_echo, GPIONUM_ECHO);
    if(gpioline_echo == NULL)
    {
        printf("gpiod_chip_get_line error\n");
        return -1;
    }

    /* 设置trig脚为输出模式，初始电平为低 */
    ret = gpiod_line_request_output(gpioline_trig, "gpioline_trig", 0);   
    if(ret < 0)
    {
        printf("gpiod_line_request_output error : gpioline_trig\n");
        return -1;
    }

    /* 设置echo脚为输入模式 */
    ret = gpiod_line_request_input(gpioline_echo, "gpioline_echo");   
    if(ret < 0)
    {
        printf("gpiod_line_request_input error : gpioline_echo\n");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    struct timespec start_ts, end_ts;  
    long long start_time, end_time;  
    float dist;  
    int bit;
    int ret;

    ret = gpio_init();
    if(ret < 0)
        return ret;

    while(1)
    {
        gpiod_line_set_value(gpioline_trig, 0);
        gpiod_line_set_value(gpioline_trig, 1);
        usleep(10);
        gpiod_line_set_value(gpioline_trig, 0);
 
        while (gpiod_line_get_value(gpioline_echo) == 0);
        clock_gettime(CLOCK_MONOTONIC, &start_ts);  
        start_time = start_ts.tv_sec * 1000000000LL + start_ts.tv_nsec;  
  
        while (gpiod_line_get_value(gpioline_echo) == 1);
        clock_gettime(CLOCK_MONOTONIC, &end_ts);  
        end_time = end_ts.tv_sec * 1000000000LL + end_ts.tv_nsec;  
  
        float duration = (end_time - start_time) / 1.0e9f;  
        dist = duration * 34300 / 2.0;  
  
        printf("Distance: %.2f cm\n", dist);

        sleep(0.1);
    }
    
    return 0;
}