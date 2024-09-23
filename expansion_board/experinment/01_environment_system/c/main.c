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
#include "config.h"

/* dht11设备名 */
#define DEV_NAME    "/dev/dht11"

/* 温湿度阈值 */
#define MAX_TEMP    (35)
#define MAX_HUMI    (60)

int dht11_fd;

static void sigint_handler(int sig_num) 
{    
    /* 关闭led，反初始化led */
    led_off(LED_RED);
    led_off(LED_GREEN);
    led_release();

    /* 关闭蜂鸣器，反初始化蜂鸣器 */
    buzzer_off();
    buzzer_release();

    /* 关闭电机驱动板，反初始化电机驱动板 */
    motor_off();
    motor_release();
    
    /* oled清屏 */
    oled_clear();

    /* 关闭dht11 */
    close(dht11_fd);

    /* 释放配置文件 */
    config_free();

    exit(0);  
}

int main(int argc, char **argv)
{
    int ret = 0;
    uint8_t data[6];
    float temp, humi;
    char temp_str[50], humi_str[50];

    /* 配置文件路径 */
    const char *filename = "../configuration.json";

    /* register exit signal ( Ctrl + c ) */
    signal(SIGINT, sigint_handler);

    /* 配置文件初始化 */
    ret = config_init(filename);
    if(ret == -1)
    {
        printf("config init err!\n");
        return -1;
    }

    /* led初始化 */
    ret = led_init();
    if(ret == -1)
    {
        printf("led init err!\n");
        led_release();
        return -1;
    }

    /* 蜂鸣器初始化 */
    ret = buzzer_init();
    if(ret == -1)
    {
        printf("buzzer init err!\n");
        buzzer_release();
        return -1;
    }

    /* dht11初始化 */
	dht11_fd = open(DEV_NAME, O_RDWR);
	if(dht11_fd < 0)
	{
		printf("can not open file %s, %d\n", DEV_NAME, dht11_fd);
		return -1;
	}

    /* oled初始化 */
    cJSON *oled_bus = config_get_value("oled", "bus");
    if(oled_bus != NULL)
    {
        ret = oled_init(oled_bus->valueint);
        if(ret == -1)
        {
            printf("oled init err!\n");
            return -1;
        }
    }
    else
    {
        return -1;
    }
        
    /* 电机驱动板初始化 */
    cJSON *stby_pin_chip, *ain1_pin_chip, *ain2_pin_chip, *bin1_pin_chip, *bin2_pin_chip;
    cJSON *stby_pin_num, *ain1_pin_num, *ain2_pin_num, *bin1_pin_num, *bin2_pin_num;
    cJSON *pwm_a, *pwm_b;

    char stby[20], ain1[20], ain2[20], bin1[20], bin2[20];
    char pwma[20], pwmb[20];

    stby_pin_chip = config_get_value("motor-driver-board", "stby_pin_chip");
    ain1_pin_chip = config_get_value("motor-driver-board", "ain1_pin_chip");
    ain2_pin_chip = config_get_value("motor-driver-board", "ain2_pin_chip");
    bin1_pin_chip = config_get_value("motor-driver-board", "bin1_pin_chip");
    bin2_pin_chip = config_get_value("motor-driver-board", "bin2_pin_chip");
    stby_pin_num = config_get_value("motor-driver-board", "stby_pin_num");
    ain1_pin_num = config_get_value("motor-driver-board", "ain1_pin_num");
    ain2_pin_num = config_get_value("motor-driver-board", "ain2_pin_num");
    bin1_pin_num = config_get_value("motor-driver-board", "bin1_pin_num");
    bin2_pin_num = config_get_value("motor-driver-board", "bin2_pin_num");
    pwm_a = config_get_value("motor-driver-board", "pwma_chip");
    pwm_b = config_get_value("motor-driver-board", "pwmb_chip");

    if(stby_pin_chip == NULL || ain1_pin_chip == NULL || ain2_pin_chip == NULL || bin1_pin_chip == NULL || bin2_pin_chip == NULL)
        return -1;
    if(stby_pin_num == NULL || ain1_pin_num == NULL || ain2_pin_num == NULL || bin1_pin_num == NULL || bin2_pin_num == NULL)
        return -1;
    if(pwm_a == NULL || pwm_b == NULL)
        return -1;

    sprintf(stby, "/dev/gpiochip%s", stby_pin_chip->valuestring);
    sprintf(ain1, "/dev/gpiochip%s", ain1_pin_chip->valuestring);
    sprintf(ain2, "/dev/gpiochip%s", ain2_pin_chip->valuestring);
    sprintf(bin1, "/dev/gpiochip%s", bin1_pin_chip->valuestring);
    sprintf(bin2, "/dev/gpiochip%s", bin2_pin_chip->valuestring);
    sprintf(pwma, "pwmchip%d", pwm_a->valueint);
    sprintf(pwmb, "pwmchip%d", pwm_b->valueint);

    ret = motor_init(stby, ain1, ain2, bin1, bin2, 
                    stby_pin_num->valueint, ain1_pin_num->valueint, ain2_pin_num->valueint, bin1_pin_num->valueint, bin2_pin_num->valueint, 
                    pwma, pwmb);
    if(ret == -1)
    {
        printf("motor init err!\n");
        motor_release();
        return -1;
    }

    while(1)
    {
        /* 读取dht11温湿度数据 */
		ret = read(dht11_fd, &data, sizeof(data));	
		if(ret)
        {
            temp = data[2] + data[3] * 0.01;
            humi = data[0] + data[1] * 0.01;
        }
        else
            printf("read data from dth11 err!\n");

        //printf("temperature=%.2f humidity=%.2f\n", temp, humi);

        sprintf(temp_str, "temp: %.2f", temp);
        sprintf(humi_str, "humi: %.2f", humi);
        oled_show_string(0, 2, temp_str);
        oled_show_string(0, 4, humi_str);

        /* 温湿度阈值判断 */
        if(temp >= MAX_TEMP || humi >= MAX_HUMI)
        {
            /* 超过阈值，红色led亮，蜂鸣器工作，风扇工作 */
            led_on(LED_RED);
            led_off(LED_GREEN);
            buzzer_on();
            motor_on();
        }
        else
        {
            /* 正常状态下，绿色led亮，蜂鸣器不工作，风扇不工作 */
            led_on(LED_GREEN);
            led_off(LED_RED);
            buzzer_off();
            motor_off();
        }

        sleep(1);
    }

    close(dht11_fd);

    return 0;
}