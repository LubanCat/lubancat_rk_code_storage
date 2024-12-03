/*
*
*   file: main.c
*   update: 2024-11-11
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
#include <time.h>

#include "motor.h"
#include "oled.h"
#include "pwm.h"
#include "key.h"
#include "config.h"

/* 温湿度阈值 */
#define MAX_TEMP                (35)    
#define MAX_HUMI                (60)

struct gpiod_chip *gpiochip_ir;         // GPIOCHIP，用于红外模块    
struct gpiod_line *gpioline_ir;         // GPIOLINE，用于检测红外信号

float temp = 0;                         // 温度数据
float humi = 0;                         // 湿度数据

int dht11_fd;                           // DHT11文件描述符
pthread_t dht11_obj;                    // DHT11线程对象
int dht11_thread_stop = 0;              // DHT11线程停止标志
pthread_mutex_t dht11_mutex = PTHREAD_MUTEX_INITIALIZER;    // DHT11互斥锁

pthread_t ir_obj;                       // 红外检测线程对象
int ir_thread_stop = 0;                 // 红外线程停止标志

pwm sg90;                               // 舵机PWM对象

/*****************************
 * @brief : 信号处理函数，用于清理资源并安全退出程序
 * @param : sig_num - 信号编号
 * @return: 无返回值
 *****************************/
void sigint_handler(int sig_num) 
{    
    /* 退出dht11线程 */
    dht11_thread_stop = 1;
    pthread_join(dht11_obj, NULL);

    /* 退出红外检测线程 */
    ir_thread_stop = 1;
    pthread_join(ir_obj, NULL);

    /* oled清屏 */
    oled_clear();

    /* 关闭dht11 */
    close(dht11_fd);

    /* 按键反初始化 */
    key_exit();
    
    /* 关闭电机驱动板，反初始化电机驱动板 */
    motor_off();
    motor_release();

    /* pwm反初始化 */
    pwm_config(sg90, "duty_cycle", "500000");
    pwm_exit(sg90);

    /* 释放配置文件 */
    config_free();

    exit(0);  
}

/*****************************
 * @brief : 线程函数，用于读取DHT11温湿度数据并在OLED上显示
 * @param : arg - 线程参数，当前为NULL
 * @return: 无返回值
 *****************************/
void *dht11_thread(void *arg) 
{  
    int ret;
    uint8_t data[6];                    // 存储DHT11数据
    char temp_str[50], humi_str[50];
    float ftemp = 0, fhumi = 0;
    
    if(dht11_fd < 0)                    // 检查DHT11设备是否打开成功
        return;

    while(dht11_thread_stop != 1) 
    {  
        /* 读取dht11温湿度数据 */
		ret = read(dht11_fd, &data, sizeof(data));	
		if(ret)
        {
            pthread_mutex_lock(&dht11_mutex);
            temp = data[2] + data[3] * 0.01;    // 提取温度
            humi = data[0] + data[1] * 0.01;    // 提取湿度
            ftemp = temp;
            fhumi = humi;
            pthread_mutex_unlock(&dht11_mutex);
        }
        else
            printf("read data from dth11 err!\n");

        //printf("temperature=%.2f humidity=%.2f\n", temp, humi);

        /* 显示温湿度数据到OLED */
        sprintf(temp_str, "temp: %.2f", ftemp);
        sprintf(humi_str, "humi: %.2f", fhumi);
        oled_show_string(0, 2, temp_str);
        oled_show_string(0, 4, humi_str);

        sleep(1);
    }   
    
    printf("%s has been exit!\n", __FUNCTION__);
    pthread_exit(NULL);
}

/*****************************
 * @brief : 线程函数，用于检测红外模块并控制舵机旋转
 * @param : arg - 线程参数，当前为NULL
 * @return: 无返回值
 *****************************/
void *ir_thread(void *arg)
{
    int ir_bit = 0;

    while(ir_thread_stop != 1)
    {
        /* 检测红外模块是否被触发 */
        ir_bit = gpiod_line_get_value(gpioline_ir);
        if(ir_bit == 0)
        {
            /* 控制舵机旋转45度 */
            printf("sg90 turn to 45 degree!\n");
            pwm_config(sg90, "duty_cycle", "1000000");      // 1ms 45度

            sleep(2);

            /* 控制舵机复位到0度 */
            printf("sg90 turn to zero degree!\n");
            pwm_config(sg90, "duty_cycle", "500000");       // 0.5ms 0度

            sleep(2);
        }      
        
        sleep(0.3);
    }

    printf("%s has been exit!\n", __FUNCTION__);
    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    int ret = 0;
    float ftemp = 0, fhumi = 0;
    int key_value;                          // 按键值
    int motor_sw = 0;                       // 风扇使能，1开启控制，0不可控制

    signal(SIGINT, sigint_handler);         // 注册SIGINT信号处理函数

    /* 初始化配置文件 */
    ret = config_init(CONFIG_FILE_NAME);
    if(ret < 0)
    {
        printf("config init error!\n");
        return -1;
    }


    /* oled初始化 */
    cJSON *oled_bus = config_get_value("oled", "bus");
    if(oled_bus == NULL)
        return -1;

    ret = oled_init(oled_bus->valueint);
    if(ret != 0)
    {
        fprintf(stderr, "oled init error!\n");
        return -1;
    }
    

    /* 板载按键初始化 */
    cJSON *key_event = config_get_value("key", "event");
    if(key_event == NULL)
        return -1;

    ret = key_init(key_event->valuestring);
    if(ret == -1)
    {
        fprintf(stderr, "key init error!\n");
        return -1;
    }
    

    /* dht11初始化 */
    cJSON *dht11_dev = config_get_value("dht11", "devname");
    if(dht11_dev == NULL)
        return -1;

    dht11_fd = open(dht11_dev->valuestring, O_RDWR);
    if(dht11_fd < 0)
    {
        fprintf(stderr, "dht11 init error!\n");
        return -1;
    }
    

    /* 电机驱动模块初始化 */
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
        fprintf(stderr, "motor init err!\n");
        motor_release();
        return -1;
    }

    if(motor_sw)
        motor_on();
    else
        motor_off();
    

    /* GPIO引脚初始化，用于红外信号检测 */
    char ir_pin_chip[20];
    cJSON *irchip = config_get_value("ir", "ir_pin_chip");
    cJSON *irnum = config_get_value("ir", "ir_pin_num");
    if(irchip == NULL || irnum == NULL)
        return -1;

    // 获取gpio控制器
    sprintf(ir_pin_chip, "/dev/gpiochip%s", irchip->valuestring);
    gpiochip_ir = gpiod_chip_open(ir_pin_chip);  
    if(gpiochip_ir == NULL)
    {
        fprintf(stderr, "open gpiochip_ir error!\n");
        return -1;
    }

    // 获取gpio引脚 
    gpioline_ir = gpiod_chip_get_line(gpiochip_ir, irnum->valueint);
    if(gpioline_ir == NULL)
    {
        fprintf(stderr, "get gpioline_ir!\n");
        return -1;
    }

    // 设置gpio引脚为输入模式
    ret = gpiod_line_request_input(gpioline_ir, "ir"); 
    if(ret < 0)
    {
        fprintf(stderr, "set ir pin to input mode error!\n");
        return -1;
    }
    

    /* 舵机PWM初始化 */
    char pwmchip[10];
    cJSON *sg90_pwm = config_get_value("sg90", "pwm_chip");
    if(sg90_pwm == NULL)
        return -1;
    sprintf(pwmchip, "pwmchip%d", sg90_pwm->valueint);

    snprintf(sg90.pwmchip, sizeof(sg90.pwmchip), "%s", pwmchip);
    snprintf(sg90.channel, sizeof(sg90.channel), "%s", "0");
    ret = pwm_init(sg90);
    if(ret == -1)
    {
        fprintf(stderr, "SG90 init err!\n");
        return -1;
    }
    pwm_config(sg90, "period", "20000000");                 // 20ms
    pwm_config(sg90, "duty_cycle", "500000");               // 0.5ms 0度
    pwm_config(sg90, "enable", "1");
    

    /* 创建DHT11读取线程 */
    ret = pthread_create(&dht11_obj, NULL, dht11_thread, NULL);  
    if(ret < 0)
    {
        fprintf(stderr, "dht11_thread create error!\n");
        return -1;
    }


    /* 创建红外检测线程 */
    ret = pthread_create(&ir_obj, NULL, ir_thread, NULL);  
    if(ret < 0)
    {
        fprintf(stderr, "ir_obj create error!\n");
        return -1;
    }
    

    while(1)
    {
        /* 获取当前温湿度数据 */
        pthread_mutex_lock(&dht11_mutex);
        ftemp = temp;
        fhumi = humi;
        pthread_mutex_unlock(&dht11_mutex);

        // printf("temp : %.2f, ", ftemp);
        // printf("humi : %.2f\n", fhumi);

        key_value = key_get_value();            // 获取按键值
        if(key_value == KEY1_PRESSED)
        {
            motor_sw = !motor_sw;               // 切换电机状态

            if(motor_sw)
            {
                // 开启电机
                printf("motor enable!\n");
                motor_on();                     // 开启电机
            }
            else
            {
                // 关闭电机
                printf("motor disabled!\n");
                motor_off();                    // 关闭电机
                continue;
            }
        }
        
        // if(fhumi > 50)
        if(ftemp > MAX_TEMP)                    // 温度超过阈值
        {
            // 加速旋转
            motor_pwmA_config("duty_cycle", "600");
        }
        else 
        {
            // 匀速旋转
            motor_pwmA_config("duty_cycle", "500");
        }
        
        sleep(1);
    }

    return 0;
}