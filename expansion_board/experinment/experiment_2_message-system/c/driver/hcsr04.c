/*
*
*   file: hc_sr04.c
*   update: 2024-09-12
*
*/

#include "hcsr04.h"

static struct gpiod_chip *gpiochip_trig;      
static struct gpiod_chip *gpiochip_echo;

static struct gpiod_line *gpioline_trig;          
static struct gpiod_line *gpioline_echo;

static int init_flag = 0;

static float distance_buffer[BUFFER_SIZE];                     // 存储测量结果的缓冲区  
static int buffer_index = 0;                                   // 缓冲区索引

/*****************************
 * @brief : hc_sr04初始化
 * @param : trig_gpiochip   trig引脚的gpiochip
 * @param : trig_gpionum    trig引脚的gpionum
 * @param : echo_gpiochip   echo引脚的gpiochip
 * @param : echo_gpionum    echo引脚的gpionum
 * @return: 0成功 1失败
*****************************/
int hc_sr04_init(const char *trig_gpiochip, unsigned int trig_gpionum, const char *echo_gpiochip, unsigned int echo_gpionum)
{
    int ret;

    if(init_flag)
        return 0;

    /* get gpio controller */
    gpiochip_trig = gpiod_chip_open(trig_gpiochip);  
    if(gpiochip_trig == NULL)
    {
        printf("gpiod_chip_open error\n");
        return -1;
    }

    gpiochip_echo = gpiod_chip_open(echo_gpiochip);  
    if(gpiochip_echo == NULL)
    {
        printf("gpiod_chip_open error\n");
        return -1;
    }

    /* get gpio line */
    gpioline_trig = gpiod_chip_get_line(gpiochip_trig, trig_gpionum);
    if(gpioline_trig == NULL)
    {
        printf("gpiod_chip_get_line error\n");
        return -1;
    }

    gpioline_echo = gpiod_chip_get_line(gpiochip_echo, echo_gpionum);
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

    init_flag = 1;
    return 0;
}

/*****************************
 * @brief : hc_sr04获取测距距离
 * @param : none
 * @return: 返回当前测距距离
*****************************/
float hc_sr04_get_distance()
{
    struct timespec start_ts, end_ts;  
    long long start_time, end_time;  
    float dist;

    float avg_dist = 0.0;  
    int count = 0;

    if(!init_flag)
        return 0.0;

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

    return avg_dist;
}

/*****************************
 * @brief : hc_sr04反初始化
 * @param : none
 * @return: none
*****************************/
void hc_sr04_exit()
{
    if(!init_flag)
        return;

    gpiod_line_release(gpioline_trig);
    gpiod_line_release(gpioline_echo);

    gpiod_chip_close(gpiochip_trig);
    gpiod_chip_close(gpiochip_echo);

    init_flag = 0;
}