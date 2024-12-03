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
#include "key.h"
#include "config.h"
#include "ec11.h"

menu_t *menu_Host;

extern void hcsr04_info_func(void **params);
extern void dht11_info_func(void *params);
extern void mpu6050_gyro_info_func(void **params);
extern void mpu6050_accel_info_func(void **params);
extern void max30102_info_func(void **params);
extern void ldr_ntc_info_func(void **params);

/*****************************
 * @brief : 信号处理函数，用于处理退出信号，进行资源的反初始化
 * @param : sig_num 信号编号
 *****************************/
void sigint_handler(int sig_num) 
{    
    /* OLED反初始化 */
    oled_clear();

    /* EC11旋转编码器反初始化 */
    ec11_thread_stop();
    ec11_exit();

    /* 板载按键反初始化 */
    key_exit();

    /* 释放菜单 */
    menu_free(menu_Host);

    exit(0);  
}

/*****************************
 * @brief : 创建菜单
 * @param : 无
 * @return: 成功返回0，失败返回-1
 *****************************/
int create_menu()
{   
    /*  菜单结构
    *   |- Message
    *       |- ultrasonic
    *       |- DHT11
    *       |- MPU6050
    *           |- accel
    *           |- gyro
    *       |- MAX30102
    *       |- LDR-NTC
    */

    int ret;

    /* 为MPU6050子菜单添加两个子菜单 */
    /*  */
    menu_t *menu_mpu6050 = menu_init("MPU6050");
    if(menu_mpu6050 == NULL)
    {
        fprintf(stderr, "menu_mpu6050 menu_init err!\n");
        return -1;
    }
    /* 创建加速度子菜单 */
    item_info_t *info_accel = item_info_init("accel", NULL, mpu6050_accel_info_func, &menu_Host);
    if(info_accel == NULL)
    {
        fprintf(stderr, "info_accel item_info_init err!\n");
        return -1;
    }
    /* 创建陀螺仪子菜单 */
    item_info_t *info_gyro = item_info_init("gyro", NULL, mpu6050_gyro_info_func, &menu_Host);
    if(info_gyro == NULL)
    {
        fprintf(stderr, "info_gyro item_info_init err!\n");
        return -1;
    }
    /* 将加速度、陀螺仪子菜单项加入MPU6050菜单 */
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

    /* 1、创建总菜单Message */
    menu_t *menu_message = menu_init("Message");
    if(menu_message == NULL)
    {
        fprintf(stderr, "menu_message menu_init err!\n");
        return -1;
    }

    /* 2、超声波菜单 */
    /* 创建超声波子菜单 */
    item_info_t *info_ultrasonic = item_info_init("ultrasonic", NULL, hcsr04_info_func, &menu_Host);
    if(info_ultrasonic == NULL)
    {
        fprintf(stderr, "info_ultrasonic item_info_init err!\n");
        return -1;
    }
    /* 将 超声波菜单 添加到 总菜单Message */
    ret = menu_add_item_info(menu_message, info_ultrasonic);
    if(ret == -1)
    {
        fprintf(stderr, "menu_message add info_ultrasonic err!\n");
        return -1;
    }

    /* 3、DHT11菜单 */
    /* 创建DHT11子菜单 */
    item_info_t *info_dht11 = item_info_init("dht11", NULL, dht11_info_func, &menu_Host);
    if(info_dht11 == NULL)
    {
        fprintf(stderr, "info_dht11 item_info_init err!\n");
        return -1;
    }
    /* 将 DHT11子菜单 添加到 总菜单Message */
    ret = menu_add_item_info(menu_message, info_dht11);
    if(ret == -1)
    {
        fprintf(stderr, "menu_message add info_dht11 err!\n");
        return -1;
    }

    /* 4、MPU6050菜单 */
    /* 创建MPU6050子菜单 */
    item_info_t *info_mpu6050 = item_info_init("MPU6050", menu_mpu6050, NULL, NULL);
    if(info_mpu6050 == NULL)
    {
        fprintf(stderr, "info_mpu6050 item_info_init err!\n");
        return -1;
    }
    /* 将 MPU6050子菜单 添加到 总菜单Message */
    ret = menu_add_item_info(menu_message, info_mpu6050);
    if(ret == -1)
    {
        fprintf(stderr, "menu_message add info_mpu6050 err!\n");
        return -1;
    }

    /* 5、MAX30102菜单 */
    /* 创建MAX30102子菜单 */
    item_info_t *info_max30102 = item_info_init("MAX30102", NULL, max30102_info_func, &menu_Host);
    if(info_max30102 == NULL)
    {
        fprintf(stderr, "info_max30102 item_info_init err!\n");
        return -1;
    }
    /* 将 MAX30102子菜单 添加到 总菜单Message */
    ret = menu_add_item_info(menu_message, info_max30102);
    if(ret == -1)
    {
        fprintf(stderr, "menu_message add info_max30102 err!\n");
        return -1;
    }

    /* 6、光敏热敏菜单 */
    /* 创建光敏热敏子菜单 */
    item_info_t *info_ldrntc = item_info_init("LDR-NTC", NULL, ldr_ntc_info_func, &menu_Host);
    if(info_ldrntc == NULL)
    {
        fprintf(stderr, "info_ldrntc item_info_init err!\n");
        return -1;
    }
    /* 将 光敏热敏子菜单 添加到 总菜单Message */
    ret = menu_add_item_info(menu_message, info_ldrntc);
    if(ret == -1)
    {
        fprintf(stderr, "menu_message add info_ldrntc err!\n");
        return -1;
    }

    menu_Host = menu_message;
    menu_print(menu_Host);

    return 0;
    
}

/*****************************
 * @brief : 显示当前菜单
 * @param : 无
 * @return: 无
 *****************************/
void display_menu()
{
    int start_x = 0;

    int len = 0;
    unsigned char *menu_title = menu_Host->title;
    unsigned char *info_name = menu_Host->info[menu_Host->current_page - 1]->name;

    char arrow[2] = {'<', '>'};
    unsigned int arrow_offset = 10;

    unsigned char page_mes[10];
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

    signal(SIGINT, sigint_handler);

    /* 配置文件初始化 ***********************************/
    ret = config_init(CONFIG_FILE_NAME);
    if(ret < 0)
    {
        printf("config init error!\n");
        return -1;
    }
    /***************************************************/

    /* oled初始化 **************************************/
    cJSON *oled_bus = config_get_value("oled", "bus");
    if(oled_bus == NULL)
        return -1;

    ret = oled_init(oled_bus->valueint);
    if(ret != 0)
    {
        fprintf(stderr, "oled init error!\n");
        return -1;
    }
    /***************************************************/

    /* EC11旋转编码器初始化 *****************************/
    cJSON *ec11_sw_event = config_get_value("ec11", "sw-event");
    cJSON *ec11_a_event = config_get_value("ec11", "a-event");
    cJSON *ec11_b_event = config_get_value("ec11", "b-event");
    if(ec11_sw_event == NULL || ec11_a_event == NULL || ec11_b_event == NULL)
        return -1;
    
    ret |= ec11_init(ec11_sw_event->valuestring, ec11_a_event->valuestring, ec11_b_event->valuestring);
    ret |= ec11_thread_start();
    if(ret != 0)
    {
        fprintf(stderr, "ec11 init error!\n");
        return -1;
    }   
    /***************************************************/

    /* 板载按键初始化 ***********************************/
    cJSON *key_event = config_get_value("key", "event");
    if(key_event == NULL)
        return -1;

    ret = key_init(key_event->valuestring);
    if(ret == -1)
    {
        fprintf(stderr, "key init error!\n");
        return -1;
    }
    /***************************************************/

    /* 创建菜单目录 *************************************/
    ret = create_menu();
    if(ret < 0)
    {
        fprintf(stderr, "create menu err\n");
        return -1;
    }
    display_menu();
    /***************************************************/

    while(1)
    {
        sleep(0.2);

        /* 更新按键状态 */
        input_value = key_get_value();              
        if(input_value != KEY_NORMAL)
            goto inspect;

        input_value = ec11_get_value();             
        if(input_value == EC11_NORMAL)
            continue;
        
inspect:
        switch (input_value) 
        {  
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