/*
*
*   file: menu_mpu6050.c
*   update: 2024-11-02
*
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "oled.h"
#include "menu.h"
#include "key.h"
#include "config.h"
#include "mpu6050.h"

void mpu6050_accel_info_func(void **params)
{
    int ret;
    short int accel_buf[3];
    float lsb_sensitivity_accel = 8192;     

    char accel_x_str[20];
    char accel_y_str[20];
    char accel_z_str[20];

    if(params == NULL)
        goto _exit;
    menu_t *menu_Host = (menu_t *)(*params);
    
    /* 1、清屏 */
    oled_clear();

    /* 2、显示菜单项名称 */
    unsigned char *info_name = menu_Host->info[menu_Host->current_page - 1]->name;
    int len = strlen(info_name);
    int start_x = (128 * 0.5) - (len * 8 * 0.5);
    oled_show_string(start_x, 0, info_name);

    /* 3、MPU6050初始化 */
    cJSON *mpu6050_bus = config_get_value("mpu6050", "bus");
    if(mpu6050_bus == NULL)
        goto _exit;
    ret = mpu6050_init(mpu6050_bus->valueint);
    if(ret < 0)
    {
        fprintf(stderr, "mpu6050 init err\n");
        oled_show_string(0, 3, "no sensor...");

        while(key_get_value() != KEY2_PRESSED)
            usleep(1000);

        goto _exit;
    }
    
    oled_show_string(70, 2, "m/s^2");
    oled_show_string(70, 4, "m/s^2");
    oled_show_string(70, 6, "m/s^2");

    /* 4、循环显示加速度数据 */
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

_exit:

    /* 5、MPU6050反初始化 */
    mpu6050_exit();

    oled_clear();
}

void mpu6050_gyro_info_func(void **params)
{   
    int ret;
    short int gyro_buf[3];
    float lsb_sensitivity_gyro = 16.4;    

    char gyro_x_str[20];
    char gyro_y_str[20];
    char gyro_z_str[20];

    if(params == NULL)
        goto _exit;
    menu_t *menu_Host = (menu_t *)(*params);
    
    /* 1、清屏 */
    oled_clear();

    /* 2、显示菜单项名称 */
    unsigned char *info_name = menu_Host->info[menu_Host->current_page - 1]->name;
    int len = strlen(info_name);
    int start_x = (128 * 0.5) - (len * 8 * 0.5);
    oled_show_string(start_x, 0, info_name);

    /* 3、MPU6050初始化 */
    cJSON *mpu6050_bus = config_get_value("mpu6050", "bus");
    if(mpu6050_bus == NULL)
        goto _exit;
    ret = mpu6050_init(mpu6050_bus->valueint);
    if(ret < 0)
    {
        fprintf(stderr, "mpu6050 init err\n");
        oled_show_string(0, 3, "no sensor...");

        while(key_get_value() != KEY2_PRESSED)
            usleep(1000);

        goto _exit;
    }
    
    oled_show_string(70, 2, "(rad/s)");
    oled_show_string(70, 4, "(rad/s)");
    oled_show_string(70, 6, "(rad/s)");

    /* 4、循环显示陀螺仪数据 */
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

_exit:

    /* 5、MPU6050反初始化 */
    mpu6050_exit();

    oled_clear();
}