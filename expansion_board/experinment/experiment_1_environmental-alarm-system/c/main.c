/*
*
*   file: main.c
*   update: 2024-10-17
*   usage: 
*       make
*       sudo ./main
*
*/

#include <stdio.h>  
#include <stdlib.h>  
#include <fcntl.h>  
#include <unistd.h>
#include <signal.h> 
#include <pthread.h>  
#include <gpiod.h>
#include <linux/input.h>

#include "key.h"
#include "config.h"

struct gpiod_chip *r_led_gpiochip;   
struct gpiod_chip *g_led_gpiochip;
struct gpiod_chip *b_led_gpiochip;
struct gpiod_line *r_led_line;          
struct gpiod_line *g_led_line;       
struct gpiod_line *b_led_line;

struct gpiod_chip *ldr_gpiochip;   
struct gpiod_chip *ntc_gpiochip;
struct gpiod_chip *flame_gpiochip;
struct gpiod_line *ldr_line;          
struct gpiod_line *ntc_line;  
struct gpiod_line *flame_line;

struct gpiod_chip *buzzer_gpiochip;   
struct gpiod_line *buzzer_line; 

pthread_t led_heart_beat_p, led_brightness_p, led_alarm_p;

/*****************************
 * @brief : 传感器模块初始化（热敏模块、光敏模块、火焰模块）
 * @param : none
 * @return: 0 成功, -1 失败
*****************************/
int sensor_modules_init()
{
    int ret;

    // 从配置文件中获取热敏模块、光敏模块、火焰模块引脚信息
    char ldr_pin_chip[20], ntc_pin_chip[20], flame_pin_chip[20];
    cJSON *ldrchip = config_get_value("ldr", "pin_chip");
    cJSON *ldrnum = config_get_value("ldr", "pin_num");
    cJSON *ntcchip = config_get_value("ntc", "pin_chip");
    cJSON *ntcnum = config_get_value("ntc", "pin_num");
    cJSON *flamechip = config_get_value("flame", "pin_chip");
    cJSON *flamenum = config_get_value("flame", "pin_num");
    if(ldrchip == NULL || ntcchip == NULL || flamechip == NULL || ldrnum == NULL || ntcnum == NULL || flamenum == NULL)
        return -1;

    // 获取gpio控制器
    sprintf(ldr_pin_chip, "/dev/gpiochip%s", ldrchip->valuestring);
    sprintf(ntc_pin_chip, "/dev/gpiochip%s", ntcchip->valuestring);
    sprintf(flame_pin_chip, "/dev/gpiochip%s", flamechip->valuestring);
    ldr_gpiochip = gpiod_chip_open(ldr_pin_chip);  
    ntc_gpiochip = gpiod_chip_open(ntc_pin_chip);
    flame_gpiochip = gpiod_chip_open(flame_pin_chip);
    if(ldr_gpiochip == NULL || ntc_gpiochip == NULL || flame_gpiochip == NULL)
    {
        fprintf(stderr, "gpiod_chip_open error!\n");
        return -1;
    }

    // 获取gpio引脚 
    ldr_line = gpiod_chip_get_line(ldr_gpiochip, ldrnum->valueint);
    ntc_line = gpiod_chip_get_line(ntc_gpiochip, ntcnum->valueint);
    flame_line = gpiod_chip_get_line(flame_gpiochip, flamenum->valueint);
    if(ldr_line == NULL || ntc_line == NULL || flame_line == NULL)
    {
        fprintf(stderr, "gpiod_chip_get_line error!\n");
        return -1;
    }

    // 设置引脚为输入模式
    ret = gpiod_line_request_input(ldr_line, "ldr"); 
    ret = gpiod_line_request_input(ntc_line, "ntc"); 
    ret = gpiod_line_request_input(flame_line, "flame"); 
    if(ret < 0)
    {
        printf("set sensor_line to input mode error!\n");
        return -1;
    }

    return 0;
}

/*****************************
 * @brief : 传感器模块反初始化（热敏模块、光敏模块、火焰模块）
 * @param : none
 * @return: none
*****************************/
void sensor_modules_exit()
{
    if(ldr_line)
    {
        gpiod_line_release(ldr_line);
        ldr_line = NULL;
    }
    if(ntc_line)
    {
        gpiod_line_release(ntc_line);
        ntc_line = NULL;
    }
    if(flame_line)
    {
        gpiod_line_release(flame_line);
        flame_line = NULL;
    }
    if(ldr_gpiochip)
    {
        gpiod_chip_close(ldr_gpiochip);
        ldr_gpiochip = NULL;
    }
    if(ntc_gpiochip)
    {
        gpiod_chip_close(ntc_gpiochip);
        ntc_gpiochip = NULL;
    }
    if(flame_gpiochip)
    {
        gpiod_chip_close(flame_gpiochip);
        flame_gpiochip = NULL;
    }
}

/*****************************
 * @brief : LED初始化
 * @param : none
 * @return: 0 成功, -1 失败
*****************************/
int led_init()
{
    int ret;

    // 从配置文件中获取led引脚信息
    char ledr_pin_chip[20], ledg_pin_chip[20], ledb_pin_chip[20];
    cJSON *ledrchip = config_get_value("led-r", "pin_chip");
    cJSON *ledrnum = config_get_value("led-r", "pin_num");
    cJSON *ledgchip = config_get_value("led-g", "pin_chip");
    cJSON *ledgnum = config_get_value("led-g", "pin_num");
    cJSON *ledbchip = config_get_value("led-b", "pin_chip");
    cJSON *ledbnum = config_get_value("led-b", "pin_num");
    if(ledrchip == NULL || ledgchip == NULL || ledbchip == NULL || ledrnum == NULL || ledgnum == NULL || ledbnum == NULL)
        return -1;

    // 获取gpio控制器
    sprintf(ledr_pin_chip, "/dev/gpiochip%s", ledrchip->valuestring);
    sprintf(ledg_pin_chip, "/dev/gpiochip%s", ledgchip->valuestring);
    sprintf(ledb_pin_chip, "/dev/gpiochip%s", ledbchip->valuestring);
    r_led_gpiochip = gpiod_chip_open(ledr_pin_chip);  
    g_led_gpiochip = gpiod_chip_open(ledg_pin_chip);
    b_led_gpiochip = gpiod_chip_open(ledb_pin_chip);
    if(r_led_gpiochip == NULL || g_led_gpiochip == NULL || b_led_gpiochip == NULL)
    {
        fprintf(stderr, "gpiod_chip_open error!\n");
        return -1;
    }

    // 获取gpio引脚 
    r_led_line = gpiod_chip_get_line(r_led_gpiochip, ledrnum->valueint);
    g_led_line = gpiod_chip_get_line(g_led_gpiochip, ledgnum->valueint);
    b_led_line = gpiod_chip_get_line(b_led_gpiochip, ledbnum->valueint);
    if(r_led_line == NULL || g_led_line == NULL || b_led_line == NULL)
    {
        fprintf(stderr, "gpiod_chip_get_line error!\n");
        return -1;
    }

    // 设置引脚为输出模式
    ret = gpiod_line_request_output(r_led_line, "ledr", 1); 
    ret = gpiod_line_request_output(g_led_line, "ledg", 1); 
    ret = gpiod_line_request_output(b_led_line, "ledb", 1); 
    if(ret < 0)
    {
        printf("set led_line to output mode error!\n");
        return -1;
    }

    return 0;
}

/*****************************
 * @brief : LED反初始化
 * @param : none
 * @return: none
*****************************/
void led_exit()
{
    if (r_led_line) 
    {
        gpiod_line_set_value(r_led_line, 1);
        gpiod_line_release(r_led_line);
        r_led_line = NULL;
    }
    if (g_led_line) 
    {
        gpiod_line_set_value(g_led_line, 1);
        gpiod_line_release(g_led_line);
        g_led_line = NULL;
    }
    if (b_led_line) 
    {
        gpiod_line_set_value(b_led_line, 1);
        gpiod_line_release(b_led_line);
        b_led_line = NULL;
    }
    if (r_led_gpiochip)
    {
        gpiod_chip_close(r_led_gpiochip);
        r_led_gpiochip = NULL;
    }
    if (g_led_gpiochip)
    {
        gpiod_chip_close(g_led_gpiochip);
        g_led_gpiochip = NULL;
    }
    if (b_led_gpiochip)
    {
        gpiod_chip_close(b_led_gpiochip);
        b_led_gpiochip = NULL;
    }
}

/*****************************
 * @brief : 蜂鸣器初始化
 * @param : none
 * @return: 0 成功, -1 失败
*****************************/
int buzzer_init()
{
    int ret;

    // 从配置文件中获取蜂鸣器引脚信息
    char buzzer_pin_chip[20];
    cJSON *buzzerchip = config_get_value("buzzer", "pin_chip");
    cJSON *buzzernum = config_get_value("buzzer", "pin_num");
    if(buzzerchip == NULL || buzzernum == NULL)
        return -1;

    // 获取gpio控制器
    sprintf(buzzer_pin_chip, "/dev/gpiochip%s", buzzerchip->valuestring);
    buzzer_gpiochip = gpiod_chip_open(buzzer_pin_chip);  
    if(buzzer_gpiochip == NULL)
    {
        fprintf(stderr, "gpiod_chip_open error!\n");
        return -1;
    }

    // 获取gpio引脚 
    buzzer_line = gpiod_chip_get_line(buzzer_gpiochip, buzzernum->valueint);
    if(buzzer_line == NULL)
    {
        fprintf(stderr, "gpiod_chip_get_line error!\n");
        return -1;
    }

    // 设置引脚为输出模式
    ret = gpiod_line_request_output(buzzer_line, "buzzer", 0); 
    if(ret < 0)
    {
        printf("set buzzer_line to output mode error!\n");
        return -1;
    }

    return 0;
}

/*****************************
 * @brief : 蜂鸣器反初始化
 * @param : none
 * @return: 0 成功, -1 失败
*****************************/
void buzzer_exit()
{
    if(buzzer_line)
    {
        gpiod_line_release(buzzer_line);
        buzzer_line = NULL;
    }
    if (buzzer_gpiochip)
    {
        gpiod_chip_close(buzzer_gpiochip);
        buzzer_gpiochip = NULL;
    }
}

/*****************************
 * @brief : 设置蜂鸣器状态
 * @param : bit 0 关闭，1 打开
 * @return: none
*****************************/
void buzzer_set(int bit)
{
    if(bit)
        gpiod_line_set_value(buzzer_line, 1);
    else
        gpiod_line_set_value(buzzer_line, 0);
}

/*****************************
 * @brief : LED心跳线程
 * @param : none
 * @return: none
*****************************/
void *led_heart_beat_thread(void *arg)
{
    printf("create led heart beat thread\n");
    while (1)
    {
        gpiod_line_set_value(r_led_line, 0);    // LED1隔秒闪烁
        sleep(1);
        gpiod_line_set_value(r_led_line, 1);
        sleep(1);
        pthread_testcancel();                   // 检测线程是否应被取消
    }
    return NULL;
}

/*****************************
 * @brief : 光敏LED线程
 * @param : none
 * @return: none
*****************************/
void *led_brightness_thread(void *arg)
{
    int ldr;

    printf("create led brightness thread\n");
    while(1)
    {
        ldr = gpiod_line_get_value(ldr_line);               // 获取光敏模块的引脚状态
        if(ldr == 0)                                
            gpiod_line_set_value(g_led_line, 1);            // 当亮度达到阈值时，光敏模块DO输出低电平，LED2灭
        else
            gpiod_line_set_value(g_led_line, 0);            // 当亮度未达到阈值时，光敏模块DO输出高电平，LED2亮

        sleep(0.5);
        pthread_testcancel();                               // 检测线程是否应被取消  
    }
    return NULL;
}

/*****************************
 * @brief : 温度报警线程
 * @param : none
 * @return: none
*****************************/
void *led_alarm_thread(void *arg)
{
    int ntc, flame;

    printf("create led alarm thread\n");
    while(1)
    {
        ntc = gpiod_line_get_value(ntc_line);               // 获取热敏模块的引脚状态
        flame = gpiod_line_get_value(flame_line);           // 获取火焰模块的引脚状态
        if(ntc == 0 || flame == 0)                   
        {
            gpiod_line_set_value(b_led_line, 0);            // 当热敏模块和火焰模块达到阈值时，DO输出低电平，LED3亮
            buzzer_set(1);                                  // 当热敏模块和火焰模块达到阈值时，DO输出低电平，蜂鸣器工作
        }

        sleep(0.5);
        pthread_testcancel();                               // 检测线程是否应被取消  
    }
    return NULL;
}

/*****************************
 * @brief : SIGINT信号处理函数
 * @param : sig_num 信号编号
 * @return: none
*****************************/
void sigint_handler(int sig_num) 
{    
    printf("\n");
    printf("Signal received, cancelling threads...\n");
    pthread_cancel(led_heart_beat_p);   // 终止LED心跳线程
    pthread_cancel(led_brightness_p);   // 终止光敏LED线程
    pthread_cancel(led_alarm_p);        // 终止温度报警线程 

    pthread_join(led_heart_beat_p, NULL);
    pthread_join(led_brightness_p, NULL);
    pthread_join(led_alarm_p, NULL);
    printf("All threads cancelled. Cleaning up resources...\n");

    buzzer_set(0);                      // 关闭蜂鸣器

    led_exit();                         // 清理资源
    sensor_modules_exit();              // 清理资源 
    buzzer_exit();                      // 清理资源

    exit(0);  
}

int main(int argc, char **argv)
{
    int ret;
    int key_value;

    signal(SIGINT, sigint_handler);

    // 配置文件初始化 
    ret = config_init(CONFIG_FILE_NAME);
    if(ret < 0)
    {
        printf("config init error!\n");
        return -1;
    }

    // 板载按键初始化 
    cJSON *key_event = config_get_value("key", "event");
    if(key_event == NULL)
        return -1;

    ret = key_init(key_event->valuestring);
    if(ret == -1)
    {
        fprintf(stderr, "key init error!\n");
        return -1;
    }
    
    // LED初始化 
    ret = led_init();
    if (ret == -1)
        goto _err_exit;

    // 初始化传感器模块（热敏模块、光敏模块、火焰模块）
    ret = sensor_modules_init();
    if (ret == -1)
        goto _err_exit;

    // 初始化蜂鸣器
    ret = buzzer_init();
    if (ret == -1)
        goto _err_exit;
    
    // 创建LED心跳线程
    ret = pthread_create(&led_heart_beat_p, NULL, led_heart_beat_thread, NULL);
    if (ret != 0) 
    {
        printf("Error: Failed to create heartbeat thread\n");
        goto _err_exit;
    }

    // 创建光敏LED线程
    ret = pthread_create(&led_brightness_p, NULL, led_brightness_thread, NULL);
    if (ret != 0) 
    {
        printf("Error: Failed to create brightness thread\n");
        goto _err_exit;
    }

    // 创建温度报警线程
    ret = pthread_create(&led_alarm_p, NULL, led_alarm_thread, NULL);
    if (ret != 0) 
    {
        printf("Error: Failed to create alarm thread\n");
        goto _err_exit;
    }

    while(1)
    {
        key_value = key_get_value();
        if (key_value == KEY1_PRESSED)
        {
            gpiod_line_set_value(b_led_line, 1);        // LED3灭
            buzzer_set(0);                              // 蜂鸣器停
        }

        sleep(0.1);
    }

    return 0;

_err_exit:
    led_exit();
    sensor_modules_exit();
    buzzer_exit();
    return ret;
}
