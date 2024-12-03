/*
*
*   file: main.c
*   update: 2024-10-23
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
#include "mpu6050.h"

int oled_init_flag = 1;

void sigint_handler(int sig_num) 
{    
    /* oled清屏 */
    if(oled_init_flag != 0)
        oled_clear();

    mpu6050_exit();

    exit(0);  
}

int main(int argc, char **argv)
{
    int ret = 0;

    short int accel_buf[3];
    short int gyro_buf[3];
    
    float lsb_sensitivity_accel = 8192;
    float lsb_sensitivity_gyro = 16.4;

    char accel_x_str[20] = {0};
    char accel_y_str[20] = {0};
    char accel_z_str[20] = {0};

    char gyro_x_str[20] = {0};
    char gyro_y_str[20] = {0};
    char gyro_z_str[20] = {0};

    /* register exit signal ( Ctrl + c ) */
    signal(SIGINT, sigint_handler);

    /* oled初始化 */
    ret = oled_init(3);
    if(ret == -1)
    {
        printf("oled init err!\n");
        oled_init_flag = 0;
    }

    /* mpu6050初始化 */
    ret = mpu6050_init(5);
    if(ret == -1)
    {
        printf("mpu6050 init err!\n");
        return -1;
    }

    while(1)
    {   
        mpu6050_read_gyro(gyro_buf);
        mpu6050_read_accel(accel_buf);
        
        sprintf(accel_x_str, "%.2f0", accel_buf[0]/lsb_sensitivity_accel);
        sprintf(accel_y_str, "%.2f0", accel_buf[1]/lsb_sensitivity_accel);
        sprintf(accel_z_str, "%.2f0", accel_buf[2]/lsb_sensitivity_accel);
        
        sprintf(gyro_x_str, "%.2f0", gyro_buf[0]/lsb_sensitivity_gyro);
        sprintf(gyro_y_str, "%.2f0", gyro_buf[1]/lsb_sensitivity_gyro);
        sprintf(gyro_z_str, "%.2f0", gyro_buf[2]/lsb_sensitivity_gyro);

        if(oled_init_flag != 0)
        {
            oled_show_string(2, 0, "accel");
            oled_show_string(2, 2, accel_x_str);
            oled_show_string(2, 4, accel_y_str);
            oled_show_string(2, 6, accel_z_str);

            oled_show_string(60, 0, "gyro");
            oled_show_string(60, 2, gyro_x_str);
            oled_show_string(60, 4, gyro_y_str);
            oled_show_string(60, 6, gyro_z_str);
        }

        printf("accel: x: %s, y: %s, z: %s (m/s^2)\n", accel_x_str, accel_y_str, accel_z_str);
        printf("gyro: x: %s, y: %s, z: %s (deg/s)\n", gyro_x_str, gyro_y_str, gyro_z_str);
        printf("\n");

        sleep(0.3);
    }

    return 0;
}