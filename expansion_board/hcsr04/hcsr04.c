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

// gpionum计算方法
// group: 0-3 (代表A-D)
// gpionum = group * 8 + x
// 举例 : C1 = 2 * 8 + 1 = 17
//        B6 = 1 * 8 + 6 = 14 
//
#define GPIOCHIP_TRIG           "/dev/gpiochip3"
#define GPIONUM_TRIC            (17)

#define GPIOCHIP_ECHO           "/dev/gpiochip3"
#define GPIONUM_ECHO            (14)

#define TIMEOUT                 2000000000ULL           // 2秒超时  
#define BUFFER_SIZE             10                      // 缓冲区大小

struct gpiod_chip *gpiochip_trig;      
struct gpiod_chip *gpiochip_echo;

struct gpiod_line *gpioline_trig;          
struct gpiod_line *gpioline_echo;

float distance_buffer[BUFFER_SIZE];                     // 存储测量结果的缓冲区  
int buffer_index = 0;                                   // 缓冲区索引

/*****************************
 * @brief : SIGINT信号处理函数
 * @param : sig_num 信号编号
 * @return: none
*****************************/
void sigint_handler(int sig_num) 
{    
    gpiod_line_release(gpioline_trig);
    gpiod_line_release(gpioline_echo);

    gpiod_chip_close(gpiochip_trig);
    gpiod_chip_close(gpiochip_echo);

    exit(0);  
}

/*****************************
 * @brief : 超声波模块初始化
 * @param : none
 * @return: 0 成功, -1 失败
*****************************/
int hcsr04_init(void)
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

    float avg_dist = 0.0;  
    int count = 0;  

    ret = hcsr04_init();
    if(ret < 0)
        return ret;

    while(1)
    {
        gpiod_line_set_value(gpioline_trig, 0);
        gpiod_line_set_value(gpioline_trig, 1);
        usleep(10);
        gpiod_line_set_value(gpioline_trig, 0);

        // 等待回声开始
        while (gpiod_line_get_value(gpioline_echo) == 0) 
        {
            clock_gettime(CLOCK_MONOTONIC, &start_ts);
            if ((start_ts.tv_sec * 1000000000LL + start_ts.tv_nsec) - start_time > TIMEOUT)     // 超时处理 
            {
                printf("Echo timeout (start)\n");
                break; 
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &start_ts);
        start_time = start_ts.tv_sec * 1000000000LL + start_ts.tv_nsec;  
        
        // 等待回声结束
        while (gpiod_line_get_value(gpioline_echo) == 1) 
        {
            clock_gettime(CLOCK_MONOTONIC, &end_ts);
            if ((end_ts.tv_sec * 1000000000LL + end_ts.tv_nsec) - start_time > TIMEOUT)         // 超时处理         
            {
                printf("Echo timeout (end)\n");
                break; 
            }
        }

        end_time = end_ts.tv_sec * 1000000000LL + end_ts.tv_nsec;  
        float duration = (end_time - start_time) / 1.0e9f;  
        dist = duration * 34300 / 2.0;  
        
        distance_buffer[buffer_index] = dist;               // 将新测量结果添加到缓冲区中，并计算平均值
        buffer_index = (buffer_index + 1) % BUFFER_SIZE;    // 更新索引，实现环形缓冲区  
  
        for (int i = 0; i < BUFFER_SIZE; i++)  
        {  
            avg_dist += distance_buffer[i];  
            if (distance_buffer[i] != 0)                      
                count++;  
        }  
        avg_dist /= count;                                  
  
        printf("Average Distance: %.2f cm\n", avg_dist);  

        avg_dist = 0;
        count = 0;

        usleep(100000); 
    }
    
    return 0;
}