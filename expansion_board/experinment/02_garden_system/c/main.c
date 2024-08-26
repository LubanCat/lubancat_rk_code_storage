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
#include "infrared.h"
#include "pwm.h"
#include "ads1115.h"
#include "bme280.h"

#define SG90_PWM_CHIP           "pwmchip1"
#define SG90_PWM_CHANNEL        "0"

static pwm sg90;
static int fd_key;

static void sigint_handler(int sig_num) 
{    
    /* pwm exit */
    pwm_config(sg90, "duty_cycle", "500000");
    pwm_exit(sg90);

    /* bme280 exit */
    bme280_exit();

    /* infrared exit */
    infrared_exit();

    /* ads1115 exit */
    ads1115_exit();

    /* oled off */
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

    int infrared = 0;
    int infrared_old = 1;

    double ads1115_vol = 0;

    double mq135_ppm = 0;
    char mq135_ppm_str[10];

    float bmp280_pres = 0;
    char bmp280_pres_str[20];

    /* register exit signal ( Ctrl + c ) */
    signal(SIGINT, sigint_handler);

    /* SG90 init */
    snprintf(sg90.pwmchip, sizeof(sg90.pwmchip), "%s", SG90_PWM_CHIP);
    snprintf(sg90.channel, sizeof(sg90.channel), "%s", SG90_PWM_CHANNEL);
    ret = pwm_init(sg90);
    if(ret == -1)
    {
        printf("SG90 init err!\n");
        return ret;
    }

    pwm_config(sg90, "period", "20000000");                 // 20ms
    pwm_config(sg90, "duty_cycle", "500000");               // 0.5ms 0度
    pwm_config(sg90, "enable", "1");

    /* bme280 init */
    ret = bme280_init();
    if(ret == -1)
    {
        printf("bme280 init err!\n");
        return -1;
    }

    /* ADS1115 init */
    ads1115_init();

    /* key init */			
    fd_key = open("/dev/input/event7", O_RDWR | O_NONBLOCK);    // 打开按键input设备节点
    if (fd_key < 0)
    {
        printf("can not open /dev/input/event7\n");
        return -1;
    }
    signal(SIGIO, sigio_key_func);                          // 注册信号函数
    fcntl(fd_key, F_SETOWN, getpid());                          
    flags = fcntl(fd_key, F_GETFL);                     
	fcntl(fd_key, F_SETFL, flags | FASYNC);                     // 使能异步通知模式

    /* infrared init */
    infrared_init();

    /* oled init */
    ret = oled_init();
    if(ret == -1)
    {
        printf("oled init err!\n");
        return -1;
    }

    while(1)
    {

        /* 获取adc电压值，计算ppm */
        ads1115_vol = ads1115_read_vol();
        mq135_ppm = mq135_cal_ppm(ads1115_vol);

        /* 获取大气压值 */
        bmp280_pres = bme280_get_pres();
        
        /* 人体红外感应，当当前值不同于上次值时才刷新屏幕 */
        infrared = infrared_get_value();
        if(infrared != infrared_old)                    
        {
            if(infrared)
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

            infrared_old = infrared;
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