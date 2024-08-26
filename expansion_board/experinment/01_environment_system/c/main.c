/*
*
*   file: main.c
*   update: 2024-08-09
*   usage: 
*       make
*       sudo ./main
*
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <stdint.h>
#include <signal.h> 

#include "led.h"
#include "buzzer.h"
#include "motor.h"
#include "oled.h"

#define DEV_NAME    "/dev/dht11"

/* 温湿度阈值 */
#define MAX_TEMP    (35)
#define MAX_HUMI    (60)

int fd;

static void sigint_handler(int sig_num) 
{    
    /* led off */
    led_off(r_led_line);
    led_off(g_led_line);
    led_release();

    /* buzzer off */
    buzzer_off();
    buzzer_release();

    /* motor off */
    motor_off();
    motor_release();
    
    /* oled off */
    oled_clear();

    /* dht11 off */
    close(fd);

    exit(0);  
}

int main(int argc, char **argv)
{
    int ret;
    uint8_t data[6];
    float temp, humi;
    char temp_str[50], humi_str[50];

    /* register exit signal ( Ctrl + c ) */
    signal(SIGINT, sigint_handler);

    /* led init */
    ret = led_init();
    if(ret == -1)
    {
        printf("led init err!\n");
        led_release();
        return -1;
    }

    /* buzzer init */
    ret = buzzer_init();
    if(ret == -1)
    {
        printf("buzzer init err!\n");
        buzzer_release();
        return -1;
    }

    /* motor init */
    ret = motor_init(0, 1, 0, 1);
    if(ret == -1)
    {
        printf("motor init err!\n");
        motor_release();
        return -1;
    }
    
    /* oled init */
    ret = oled_init();
    if(ret == -1)
    {
        printf("oled init err!\n");
        return -1;
    }

    /* open /dev/dht11 */
	fd = open(DEV_NAME, O_RDWR);
	if (fd < 0)
	{
		printf("can not open file %s, %d\n", DEV_NAME, fd);
		return -1;
	}

    while(1)
    {
        /* read data from dht11 */
		ret = read(fd, &data, sizeof(data));	
		if(ret)
        {
            temp = data[2] + data[3] * 0.01;
            humi = data[0] + data[1] * 0.01;
        }
        else
            printf("read data from dth11 err!\n");

        printf("temperature=%.2f humidity=%.2f\n", temp, humi);

        sprintf(temp_str, "temp: %.2f", temp);
        sprintf(humi_str, "humi: %.2f", humi);
        oled_show_string(0, 2, temp_str);
        oled_show_string(0, 4, humi_str);

        if(temp >= MAX_TEMP || humi >= MAX_HUMI)
        {
            led_on(r_led_line);
            led_off(g_led_line);
            buzzer_on();
            motor_on();
        }
        else
        {
            led_on(g_led_line);
            led_off(r_led_line);
            buzzer_off();
            motor_off();
        }

        sleep(1);
    }

    close(fd);

    return 0;
}