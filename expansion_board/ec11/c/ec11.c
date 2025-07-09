/*
*
*   file: ec11.c
*   date: 2024-08-29
*   usage: 
*       make
*       sudo ./ec11
*
*/

#include <stdio.h>  
#include <stdlib.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <string.h>  
#include <linux/input.h>  
#include <pthread.h>  
#include <errno.h>  
#include <signal.h>

#include "oled.h"

// 定义输入事件文件
#define EC11_SW_EVENT   "/dev/input/event7"  
#define EC11_A_EVENT    "/dev/input/event6"  
#define EC11_B_EVENT    "/dev/input/event5"  

// 线程ID
pthread_t ec11_SW_obj, ec11_A_obj, ec11_B_obj;  
pthread_t oled_obj;

// 文件描述符
int ec11_SW_fd, ec11_A_fd, ec11_B_fd;
int count = 0;                                      // 累计步数

// 编码器状态变量
int ec11_SW_value = 0;  
int ec11_A_value = 1;  
int ec11_B_value = 1;  
int ec11_direction = 0;                             // 0:不动作 1:顺时针旋转 2:逆时针旋转 3:按键按下顺时针旋转 4:按键按下逆时针旋转 5:按键按下
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;   // 互斥锁初始化

/*****************************
 * @brief : SIGINT信号处理函数
 * @param : sig_num 信号编号
 * @return: none
*****************************/
void sigint_handler(int sig_num) 
{    
    // 取消所有线程
    pthread_cancel(ec11_SW_obj);
    pthread_cancel(ec11_A_obj);
    pthread_cancel(ec11_B_obj);
    pthread_cancel(oled_obj);

    // 关闭文件描述符
    close(ec11_SW_fd);      
    close(ec11_A_fd);
    close(ec11_A_fd);

    // oled清屏
    oled_clear();           

    // 等待所有线程结束
    pthread_join(oled_obj, NULL);
    pthread_join(ec11_SW_obj, NULL);
    pthread_join(ec11_A_obj, NULL);
    pthread_join(ec11_B_obj, NULL);
    
    exit(0);  
}

/*****************************
 * @brief : 旋转编码器扫描线程
 * @param : arg 传入的文件描述符指针
 * @return: none
*****************************/
void *ec11_scan_thread(void *arg) 
{  
    int fd = *(int*)arg;        // 获取传入的文件描述符                    
    struct input_event ie;      // 输入事件结构体  
  
    while (1) 
    {  
        // 读取输入事件
        if (read(fd, &ie, sizeof(struct input_event)) == sizeof(struct input_event)) 
        {  
            if (ie.type == EV_KEY) 
            {  
                pthread_mutex_lock(&lock);      // 加锁以保护共享变量 

                // 处理按键事件
                if (fd == ec11_SW_fd && ie.code == 4) 
                {  
                    ec11_SW_value = ie.value;  
                    if (ec11_SW_value == 1 && ec11_A_value == 1 && ec11_B_value == 1)   // 按键按下
                        ec11_direction = 5;  
                } 
                // 处理编码器A的旋转事件
                else if (fd == ec11_A_fd && ie.code == 250) 
                {  
                    if (ie.value == 0 && ec11_B_value == 1)             // 顺时针旋转
                    {  
                        ec11_A_value = 0;  
                        ec11_direction = (ec11_SW_value == 1) ? 3 : 1;  // 判断按键状态  
                    } 
                    else if (ie.value == 1) 
                        ec11_A_value = 1;  
                } 
                // 处理编码器B的旋转事件
                else if (fd == ec11_B_fd && ie.code == 251) 
                {  
                    if (ie.value == 0 && ec11_A_value == 1)             // 逆时针旋转
                    {  
                        ec11_B_value = 0;  
                        ec11_direction = (ec11_SW_value == 1) ? 4 : 2;  // 判断按键状态  
                    } 
                    else if (ie.value == 1) 
                        ec11_B_value = 1;  
                }  

                pthread_mutex_unlock(&lock);    // 解锁
            }  
        }  
    }   
}  

/*****************************
 * @brief : OLED显示线程
 * @param : arg 无参数
 * @return: none
*****************************/
void *oled_display_thread(void *arg) 
{
    char count_str[5];      // 用于存储步数字符串
    int str_len = 0, old_str_len = 0;

    while(1)
    {
        pthread_mutex_lock(&lock);          // 加锁以保护共享变量

        sprintf(count_str, "%d", count);    // 格式化步数为字符串
        str_len = strlen(count_str);        // 获取当前字符串的长度
        if(str_len != old_str_len)
        {
            oled_clear_page(0);
            oled_clear_page(1);
            old_str_len = str_len;
        }

        oled_show_string(2, 0, count_str);  // 在OLED上显示步数
        pthread_mutex_unlock(&lock);        // 解锁

        usleep(100000);                     // 延迟，控制更新频率
    }
}

int main(int argc, char **argv) 
{  
    int ret;

    // 打开输入设备
    ec11_SW_fd = open(EC11_SW_EVENT, O_RDONLY);  
    ec11_A_fd = open(EC11_A_EVENT, O_RDONLY);  
    ec11_B_fd = open(EC11_B_EVENT, O_RDONLY);  
    if (ec11_SW_fd < 0 || ec11_A_fd < 0 || ec11_B_fd < 0) 
    {  
        printf("Failed to open input device\n");  
        return -1;  
    }

    // oled初始化 
    ret = oled_init(3);
    if(ret == -1)
        printf("oled init err!\n");
    else
        pthread_create(&oled_obj, NULL, oled_display_thread, NULL);     // 创建OLED显示线程
    
    pthread_create(&ec11_SW_obj, NULL, ec11_scan_thread, &ec11_SW_fd);  // 创建旋转编码器扫描线程 
    pthread_create(&ec11_A_obj, NULL, ec11_scan_thread, &ec11_A_fd);  
    pthread_create(&ec11_B_obj, NULL, ec11_scan_thread, &ec11_B_fd);  

    signal(SIGINT, sigint_handler);         // 注册信号处理函数                                         

    while (1) 
    {  
        pthread_mutex_lock(&lock);          // 加锁以保护共享变量

        switch (ec11_direction)             // 处理编码器方向
        {  
            case 1: 
                count++;
                printf("顺时针转 : %d\n", count); 
                break;  
            case 2: 
                count--;
                printf("逆时针转 : %d\n", count); 
                break;  
            case 3: 
                count++;
                printf("按键按下顺时针转 : %d\n", count); 
                break;  
            case 4: 
                count--;
                printf("按键按下逆时针转 : %d\n", count); 
                break;  
            case 5: 
                printf("按键按下\n"); 
                break;  
            default: 
                break;  
        }  

        ec11_direction = 0;                 // 重置方向

        pthread_mutex_unlock(&lock);        // 解锁

        usleep(10000); 
    }  
      
    return 0;  
}