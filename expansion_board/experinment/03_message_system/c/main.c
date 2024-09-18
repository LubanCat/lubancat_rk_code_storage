/*
*
*   file: main.c
*   update: 2024-09-09
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
#include <linux/input.h>
#include <math.h>
#include <pthread.h>
#include <time.h>

#include "oled.h"
#include "menu.h"
#include "ec11.h"
#include "hc_sr04.h"
#include "key.h"
#include "mpu6050.h"
#include "atgm332d.h"

#define MPU6050_I2C_BUS         (5)
#define OLED_I2C_BUS            (3)

#define ATGM332D_DEV            "/dev/ttyS3"

#define KEY_EVENT               "/dev/input/event7"     

#define EC11_SW_EVENT           "/dev/input/event6"  
#define EC11_A_EVENT            "/dev/input/event5"  
#define EC11_B_EVENT            "/dev/input/event3"  

#define GPIOCHIP_TRIG           "/dev/gpiochip3"
#define GPIOCHIP_ECHO           "/dev/gpiochip3"
#define GPIONUM_TRIC            (17)
#define GPIONUM_ECHO            (14)

menu_t *menu_Host;

static void sigint_handler(int sig_num) 
{    
    /* 释放菜单 */
    menu_free(menu_Host);

    /* 板载按键反初始化 */
    key_exit();

    /* 超声波反初始化 */
    hc_sr04_exit();

    /* GPS反初始化 */
    atgm332d_thread_stop();
    atgm332d_exit();

    /* ec11旋转编码器反初始化 */
    ec11_thread_stop();
    ec11_exit();

    /* oled清屏 */
    oled_clear();

    exit(0);  
}

void ultrasonic_info_func()
{
    float dist = 0;
    char *info_name = menu_Host->info[menu_Host->current_page - 1]->name;
    int len = 0;
    int start_x = 0;
    char dist_str[20];

    oled_clear();

    /* 1、显示菜单项名称 */
    len = strlen(info_name);
    start_x = (128 * 0.5) - (len * 8 * 0.5);
    oled_show_string(start_x, 0, info_name);

    while(key_get_value() != KEY2_PRESSED)
    {
        dist = hc_sr04_get_distance();
        if(dist < 10)
            sprintf(dist_str, "000%.2f cm", dist);
        else
            sprintf(dist_str, "00%.2f cm", dist);

        /* 2、显示测距距离 */
        len = strlen(dist_str);
        start_x = (128 * 0.5) - (len * 8 * 0.5);
        oled_show_string(start_x, 3, dist_str);

        sleep(0.1);
    }

    oled_clear();
}

void mpu6050_accel_info_func()
{
    short int accel_buf[3];
    float lsb_sensitivity_accel = 8192;     
    int len = 0;
    int start_x = 0;
    char *info_name = menu_Host->info[menu_Host->current_page - 1]->name;

    char accel_x_str[20];
    char accel_y_str[20];
    char accel_z_str[20];

    oled_clear();

    /* 1、显示菜单项名称 */
    len = strlen(info_name);
    start_x = (128 * 0.5) - (len * 8 * 0.5);
    oled_show_string(start_x, 0, info_name);

    oled_show_string(70, 2, "m/s^2");
    oled_show_string(70, 4, "m/s^2");
    oled_show_string(70, 6, "m/s^2");

    while(key_get_value() != KEY2_PRESSED)
    {   
        mpu6050_read_accel(accel_buf);
        sprintf(accel_x_str, "X: %.2f", accel_buf[0]/lsb_sensitivity_accel);
        sprintf(accel_y_str, "Y: %.2f", accel_buf[1]/lsb_sensitivity_accel);
        sprintf(accel_z_str, "Z: %.2f", accel_buf[2]/lsb_sensitivity_accel);

        oled_show_string(2, 2, accel_x_str);
        oled_show_string(2, 4, accel_y_str);
        oled_show_string(2, 6, accel_z_str);
                
        sleep(0.1);
    }

    oled_clear();
}

void mpu6050_gyro_info_func()
{
    short int gyro_buf[3];
    float lsb_sensitivity_gyro = 16.4;    
    int len = 0;
    int start_x = 0;
    char *info_name = menu_Host->info[menu_Host->current_page - 1]->name;

    char gyro_x_str[20];
    char gyro_y_str[20];
    char gyro_z_str[20];

    oled_clear();

    /* 1、显示菜单项名称 */
    len = strlen(info_name);
    start_x = (128 * 0.5) - (len * 8 * 0.5);
    oled_show_string(start_x, 0, info_name);

    oled_show_string(70, 2, "(rad/s)");
    oled_show_string(70, 4, "(rad/s)");
    oled_show_string(70, 6, "(rad/s)");

    while(key_get_value() != KEY2_PRESSED)
    {   
        mpu6050_read_gyro(gyro_buf);
        sprintf(gyro_x_str, "X: %.2f", gyro_buf[0]/lsb_sensitivity_gyro);
        sprintf(gyro_y_str, "Y: %.2f", gyro_buf[1]/lsb_sensitivity_gyro);
        sprintf(gyro_z_str, "Z: %.2f", gyro_buf[2]/lsb_sensitivity_gyro);

        oled_show_string(2, 2, gyro_x_str);
        oled_show_string(2, 4, gyro_y_str);
        oled_show_string(2, 6, gyro_z_str);
                
        sleep(0.1);
    }

    oled_clear();
}

void atgm332d_latlon_info_func()
{
    double lat, lon;
    char *info_name = menu_Host->info[menu_Host->current_page - 1]->name;
    int len = 0;
    int start_x = 0;
    char lat_str[20], lon_str[20];

    oled_clear();

    /* 1、显示菜单项名称 */
    len = strlen(info_name);
    start_x = (128 * 0.5) - (len * 8 * 0.5);
    oled_show_string(start_x, 0, info_name);

    while(key_get_value() != KEY2_PRESSED)
    {
        atgm332d_get_latlon(&lat, &lon);
        sprintf(lat_str, "lat:%.5f", lat);
        sprintf(lon_str, "lon:%.5f", lon);

        /* 2、显示lat */
        len = strlen(lat_str);
        start_x = (128 * 0.5) - (len * 8 * 0.5);
        oled_show_string(start_x, 3, lat_str);

        /* 3、显示lon */
        len = strlen(lon_str);
        start_x = (128 * 0.5) - (len * 8 * 0.5);
        oled_show_string(start_x, 5, lon_str);

        sleep(0.1);
    }

    oled_clear();
}

void atgm332d_time_info_func()
{
    int year, month, day, hours, min;
    char *info_name = menu_Host->info[menu_Host->current_page - 1]->name;
    int len = 0;
    int start_x = 0;
    char date_str[20], time_str[20];

    oled_clear();

    /* 1、显示菜单项名称 */
    len = strlen(info_name);
    start_x = (128 * 0.5) - (len * 8 * 0.5);
    oled_show_string(start_x, 0, info_name);

    while(key_get_value() != KEY2_PRESSED)
    {
        atgm332d_get_bjtime(&year, &month, &day, &hours, &min);
        sprintf(date_str, "%d-%d-%d", year, month, day);
        sprintf(time_str, "%d:%d", hours, min);

        /* 2、显示date */
        len = strlen(date_str);
        start_x = (128 * 0.5) - (len * 8 * 0.5);
        oled_show_string(start_x, 3, date_str);

        /* 3、显示time */
        len = strlen(time_str);
        start_x = (128 * 0.5) - (len * 8 * 0.5);
        oled_show_string(start_x, 5, time_str);

        sleep(0.1);
    }

    oled_clear();
}

int create_menu()
{   
    /*  菜单结构
    *   Message
    *       - ultrasonic
    *       - MPU6050
    *           - accel
    *           - gyro
    *       - GPS
    *           - latlon
    *           - time
    */

    int ret = 0;

    /* 2、为MPU6050子菜单项追加菜单 */
    menu_t *menu_mpu6050 = menu_init("MPU6050");
    if(menu_mpu6050 == NULL)
    {
        fprintf(stderr, "menu_mpu6050 menu_init err!\n");
        return -1;
    }
    /* 2-1、菜单MPU6050下的子菜单项 */
    /* 加速度子菜单项 */
    item_info_t *info_accel = item_info_init("accel", NULL, mpu6050_accel_info_func, NULL);
    if(info_accel == NULL)
    {
        fprintf(stderr, "info_accel item_info_init err!\n");
        return -1;
    }
    /* 陀螺仪子菜单项 */
    item_info_t *info_gyro = item_info_init("gyro", NULL, mpu6050_gyro_info_func, NULL);
    if(info_gyro == NULL)
    {
        fprintf(stderr, "info_gyro item_info_init err!\n");
        return -1;
    }
    /* 2-2、将加速度、陀螺仪子菜单项加入MPU6050菜单 */
    ret = menu_add_item_info(menu_mpu6050, info_accel);
    if(ret == -1)
    {
        fprintf(stderr, "menu_mpu6050 add info_accel err!\n");
        return -1;
    }
    ret = menu_add_item_info(menu_mpu6050, info_gyro);
    if(ret == -1)
    {
        fprintf(stderr, "menu_mpu6050 add info_gyro err!\n");
        return -1;
    }

    /* 3、为GPS子菜单项追加菜单 */
    menu_t *menu_gps = menu_init("GPS");
    if(menu_gps == NULL)
    {
        fprintf(stderr, "menu_gps menu_init err!\n");
        return -1;
    }
    /* 3-1、菜单GPS下的子菜单项 */
    /* 经纬度子菜单项 */
    item_info_t *info_latlon = item_info_init("latlon", NULL, atgm332d_latlon_info_func, NULL);
    if(info_latlon == NULL)
    {
        fprintf(stderr, "info_latlon item_info_init err!\n");
        return -1;
    }
    /* 北京时间子菜单项 */
    item_info_t *info_time = item_info_init("time", NULL, atgm332d_time_info_func, NULL);
    if(info_time == NULL)
    {
        fprintf(stderr, "info_time item_info_init err!\n");
        return -1;
    }
    /* 3-2、将经纬度、北京时间子菜单项加入GPS菜单 */
    ret = menu_add_item_info(menu_gps, info_latlon);
    if(ret == -1)
    {
        fprintf(stderr, "menu_gps add info_latlon err!\n");
        return -1;
    }
    ret = menu_add_item_info(menu_gps, info_time);
    if(ret == -1)
    {
        fprintf(stderr, "menu_gps add info_gyro err!\n");
        return -1;
    }

    /* 1、总菜单Message */
    menu_t *menu_message = menu_init("Message");
    if(menu_message == NULL)
    {
        fprintf(stderr, "menu_message menu_init err!\n");
        return -1;
    }
    /* 1-1、总菜单Message下的子菜单项 */
    /* 超声波子菜单项 */
    item_info_t *info_ultrasonic = item_info_init("ultrasonic", NULL, ultrasonic_info_func, NULL);
    if(info_ultrasonic == NULL)
    {
        fprintf(stderr, "info_ultrasonic item_info_init err!\n");
        return -1;
    }
    /* MPU6050子菜单项 */
    item_info_t *info_mpu6050 = item_info_init("MPU6050", menu_mpu6050, NULL, NULL);
    if(info_mpu6050 == NULL)
    {
        fprintf(stderr, "info_mpu6050 item_info_init err!\n");
        return -1;
    }
    /* GPS子菜单项 */
    item_info_t *info_gps = item_info_init("GPS", menu_gps, NULL, NULL);
    if(info_gps == NULL)
    {
        fprintf(stderr, "info_gps item_info_init err!\n");
        return -1;
    }
    /* 1-2、将超声波、GPS、MPU6050子菜单项加入Message菜单 */
    /* 超声波 */
    ret = menu_add_item_info(menu_message, info_ultrasonic);
    if(ret == -1)
    {
        fprintf(stderr, "menu_message add info_ultrasonic err!\n");
        return -1;
    }
    /* MPU6050 */
    ret = menu_add_item_info(menu_message, info_mpu6050);
    if(ret == -1)
    {
        fprintf(stderr, "menu_message add info_mpu6050 err!\n");
        return -1;
    }
    /* GPS */
    ret = menu_add_item_info(menu_message, info_gps);
    if(ret == -1)
    {
        fprintf(stderr, "menu_message add info_gps err!\n");
        return -1;
    }

    menu_Host = menu_message;
    menu_print(menu_Host);

    return 0;
    
}

void display_menu()
{
    int start_x = 0;

    int len = 0;
    char *menu_title = menu_Host->title;
    char *info_name = menu_Host->info[menu_Host->current_page - 1]->name;

    char arrow[2] = {'<', '>'};
    unsigned int arrow_offset = 10;

    char page_mes[10];
    unsigned int current_page = menu_Host->current_page;
    unsigned int total_page = menu_Host->total_page;

    /* 1、显示菜单标题 */
    oled_clear_page(0);
    oled_clear_page(1);
    len = strlen(menu_title);
    start_x = (128 * 0.5) - (len * 8 * 0.5);
    oled_show_string(start_x, 0, menu_title);

    /* 2、显示菜单项名称 */
    oled_clear_page(3);
    oled_clear_page(4);
    len = strlen(info_name);
    start_x = (128 * 0.5) - (len * 8 * 0.5);
    oled_show_string(start_x, 3, info_name);

    /* 3、显示左右箭头 */
    if(current_page <= 1)
        arrow[0] = ' ';
    else
        arrow[0] = '<';
    if(current_page >= total_page)
        arrow[1] = ' ';
    else    
        arrow[1] = '>';
    oled_show_char(0 + arrow_offset, 3, arrow[0]);
    oled_show_char(128 - 8 - arrow_offset, 3, arrow[1]);

    /* 4、显示页码 */
    sprintf(page_mes, "%d / %d", current_page, total_page);
    len = strlen(page_mes);
    start_x = (128 * 0.5) - (len * 8 * 0.5);
    oled_show_string(start_x, 6, page_mes);
}

int main(int argc, char **argv)
{
    int ret = 0;
    int input_value = 0;

    /* register exit signal ( Ctrl + c ) */
    signal(SIGINT, sigint_handler);

    /* 1、创建菜单目录 */
    ret = create_menu();
    if(ret == -1)
    {
        fprintf(stderr, "create_menu err\n");
        return -1;
    }

    /* 2、ec11旋转编码器初始化 */
    ret |= ec11_init(EC11_SW_EVENT, EC11_A_EVENT, EC11_B_EVENT);
    ret |= ec11_thread_start();
    if(ret != 0)
    {
        fprintf(stderr, "ec11 init err\n");
        return -1;
    }

    /* 3、板载按键初始化 */
    ret = key_init(KEY_EVENT);
    if(ret == -1)
    {
        fprintf(stderr, "key_init err\n");
        return -1;
    }

    /* 3、超声波初始化 */
    ret = hc_sr04_init(GPIOCHIP_TRIG, GPIONUM_TRIC, GPIOCHIP_ECHO, GPIONUM_ECHO);
    if(ret == -1)
    {
        fprintf(stderr, "hc_sr04_init err\n");
        return -1;
    }
    
    /* 3、mpu6050初始化 */
    ret = mpu6050_init(MPU6050_I2C_BUS);
    if(ret == -1)
    {
        fprintf(stderr, "mpu6050_init err\n");
        return -1;
    }

    /* 3、atgm332d初始化 */
    ret |= atgm332d_init(ATGM332D_DEV);
    ret |= atgm332d_thread_start();
    if(ret != 0)
    {
        fprintf(stderr, "atgm332d_init init err\n");
        return -1;
    }

    /* 3、oled初始化 */
    oled_init(OLED_I2C_BUS);
    display_menu();

    while(1)
    {
        sleep(0.2);

        input_value = key_get_value();
        if(input_value != KEY_NORMAL)
            goto inspect;

        input_value = ec11_get_value();
        if(input_value == EC11_NORMAL)
            continue;
        
inspect:
        switch (input_value) 
        {  
            case KEY1_PRESSED:
                //printf("板载key1按下\n");
                break;
            case KEY2_PRESSED:
                //printf("板载key2按下\n");
                menu_go_pre_menu(&menu_Host);
                break;
            case KEY3_PRESSED:
                //printf("板载key3按下\n");
                break;
            case EC11_TURN_RIGHT: 
                //printf("ec11顺时针转\n");
                menu_go_next_info(&menu_Host); 
                break;  
            case EC11_TURN_LEFT: 
                //printf("ec11逆时针转\n"); 
                menu_go_pre_info(&menu_Host);
                break;  
            case EC11_PRESS: 
                //printf("ec11按键按下\n"); 
                menu_go_next_menu(&menu_Host);
                break;  
            default: 
                break;    
        }  

        display_menu();
    }

    return 0;
}