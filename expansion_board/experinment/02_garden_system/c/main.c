/*
*
*   file: main.c
*   update: 2024-08-24
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

#include "oled.h"
#include "hcsr501.h"
#include "pwm.h"
#include "ads1115.h"
#include "bme280.h"
#include "config.h"

#define SG90_PWM_CHIP           "pwmchip1"
#define SG90_PWM_CHANNEL        "0"

static pwm sg90;
static int fd_key;

static void sigint_handler(int sig_num) 
{    
    /* sg90舵机复位 */
    pwm_config(sg90, "duty_cycle", "500000");
    pwm_exit(sg90);

    /* bme280反初始化 */
    bme280_exit();

    /* 人体红外模块反初始化 */
    hcsr501_exit();

    /* ads1115反初始化 */
    ads1115_exit();

    /* oled清屏 */
    oled_clear();

    /* close key */
    close(fd_key);

    exit(0);  
}

static void sigio_key_func(int sig)
{
	struct input_event event;
	while (read(fd_key, &event, sizeof(event)) == sizeof(event))
	{
        if (event.value)
        {
            switch (event.code)
            {
                case 11:    // key1
                    printf("key1 was pressed!\n");
                    pwm_config(sg90, "duty_cycle", "500000");       // 0.5ms 0度
                    break;
                case 2:     // key2
                    printf("key2 was pressed!\n");
                    pwm_config(sg90, "duty_cycle", "1000000");      // 1ms 45度
                    break;
                case 3:     // key3
                    printf("key3 was pressed!\n");
                    pwm_config(sg90, "duty_cycle", "1500000");      // 1.5ms 90度
                    break;
                default:
                    break;
            }
        }
	}
}

static double mq135_cal_ppm(double vrl_gass)
{
    float a = 5.06;                         // a b为甲苯检测中的校准常数                     
    float b = 2.46;
    double vrl_clean = 0.576028;            // 洁净空气下的平均Vrl电压值（就是adc读到的平均电压值）

    double ratio = vrl_gass / vrl_clean;    // 利用实际测量时的Vrl与洁净空气下的Vrl来代替Rs/R0
    double ppm = a * pow(ratio, b);

    return ppm;
}

int main(int argc, char **argv)
{
    int ret;
    struct input_event in_ev = {0};
    int	flags;
    int oled_start_x = 0;

    int hcsr501 = 0;
    int hcsr501_old = 1;

    double ads1115_vol = 0;

    double mq135_ppm = 0;
    char mq135_ppm_str[10];

    float bmp280_pres = 0;
    char bmp280_pres_str[20];

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

    /* oled初始化 */
    cJSON *oled_bus = config_get_value("oled", "bus");
    if(oled_bus == NULL)
        return -1;
    ret = oled_init(oled_bus->valueint);
    if(ret == -1)
    {
        printf("oled init err!\n");
        return -1;
    }

    /* SG90舵机初始化 */
    char sg90pwm[10];
    cJSON *sg90_pwm = config_get_value("sg90", "pwm_chip");
    if(sg90_pwm == NULL)
        return -1;

    sprintf(sg90pwm, "pwmchip%d", sg90_pwm->valueint);
    snprintf(sg90.pwmchip, sizeof(sg90.pwmchip), "%s", sg90pwm);
    snprintf(sg90.channel, sizeof(sg90.channel), "%s", "0");
    ret = pwm_init(sg90);
    if(ret == -1)
    {
        printf("SG90 init err!\n");
        return ret;
    }

    pwm_config(sg90, "period", "20000000");                 // 20ms
    pwm_config(sg90, "duty_cycle", "500000");               // 0.5ms 0度
    pwm_config(sg90, "enable", "1");

    /* bme280初始化 */
    char bmp280spi[20];
    cJSON *bmp280_spi = config_get_value("bmp280", "bus");
    if(bmp280_spi == NULL)
        return -1;

    sprintf(bmp280spi, "/dev/spidev%d.0", bmp280_spi->valueint);
    ret = bme280_init(bmp280spi);
    if(ret == -1)
    {
        printf("bme280 init err!\n");
        return -1;
    }

    /* ADS1115初始化 */
    cJSON *ads1115_bus = config_get_value("ads1115", "bus");
    if(ads1115_bus == NULL)
        return -1;
    ret = ads1115_init(ads1115_bus->valueint);
    if(ret == -1)
    {
        printf("ads1115 init err!\n");
        return -1;
    }

    /* 按键初始化 */			
    cJSON *key_event = config_get_value("key", "event");
    if(key_event == NULL)
        return -1;
    fd_key = open(key_event->valuestring, O_RDWR | O_NONBLOCK);     // 打开按键input设备节点
    if(fd_key < 0)
    {
        printf("can not open /dev/input/event7\n");
        return -1;
    }
    signal(SIGIO, sigio_key_func);                                  // 注册信号函数
    fcntl(fd_key, F_SETOWN, getpid());                          
    flags = fcntl(fd_key, F_GETFL);                     
	fcntl(fd_key, F_SETFL, flags | FASYNC);                         // 使能异步通知模式

    /* 人体红外模块初始化 */
    cJSON *hcsr501_pin_chip, *hcsr501_pin_num;
    char hcsr501_chip[20];
    hcsr501_pin_chip = config_get_value("hcsr501", "dout_chip");
    hcsr501_pin_num = config_get_value("hcsr501", "dout_pin");
    if(hcsr501_pin_chip == NULL || hcsr501_pin_num == NULL)
        return -1;
    sprintf(hcsr501_chip, "/dev/gpiochip%s", hcsr501_pin_chip->valuestring);
    hcsr501_init(hcsr501_chip, hcsr501_pin_num->valueint);

    while(1)
    {

        /* 获取adc电压值，计算ppm */
        ads1115_vol = ads1115_read_vol();
        mq135_ppm = mq135_cal_ppm(ads1115_vol);

        /* 获取大气压值 */
        bmp280_pres = bme280_get_pres();
        
        /* 人体红外感应，当当前值不同于上次值时才刷新屏幕 */
        hcsr501 = hcsr501_get_value();
        if(hcsr501 != hcsr501_old)                    
        {
            if(hcsr501)
            {
                oled_start_x = (128*0.5)-(4*16*0.5);
                oled_show_chinese(oled_start_x+16*0, 0, 0);  // 欢
                oled_show_chinese(oled_start_x+16*1, 0, 1);  // 迎
                oled_show_chinese(oled_start_x+16*2, 0, 2);  // 您
                oled_show_chinese(oled_start_x+16*3, 0, 3);  // 来
            }
            else
            {
                oled_clear_page(0);
                oled_clear_page(1);
            }

            hcsr501_old = hcsr501;
        }   

        /* oled刷新ppm值 */
        sprintf(mq135_ppm_str, "ppm: %.2f", mq135_ppm);
        oled_show_string(0, 2, mq135_ppm_str);

        /* oled刷新大气压值 */
        sprintf(bmp280_pres_str, "pres: %.2fKPa", bmp280_pres*0.001);
        oled_show_string(0, 4, bmp280_pres_str);

        sleep(1);
    }

    return 0;
}