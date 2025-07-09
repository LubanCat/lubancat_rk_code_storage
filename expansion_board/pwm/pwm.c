/*
*
*   file: pwm.c
*   update: 2024-08-12
*   usage: 
*       sudo gcc -o pwm pwm.c
*       sudo ./pwm
*
*/

#include <gpiod.h>
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

#define GPIOCHIP_PWM1       "pwmchip1"
#define GPIOCHIP_PWM2       "pwmchip2"

#define PWM1_CHANNEL        "0"
#define PWM2_CHANNEL        "0"

typedef struct pwm {
    char pwmchip[10];
    char channel[10];
}pwm;
pwm pwm1, pwm2;

/*****************************
 * @brief : pwm属性配置
 * @param : pwmx, pwm结构体
 * @param : attr, pwm属性
 * @param : val,  pwm属性所要设置的值
 * @return: -1配置失败 0配置成功
*****************************/
static int pwm_config(pwm pwmx, const char *attr, const char *val)
{
    char temp_path[100];
    int len;
    int fd;

    if (pwmx.pwmchip == NULL || pwmx.channel == NULL || attr == NULL || val == NULL)
    {
        printf("%s : illegal input-parameters\n", __FUNCTION__);
        return -1;
    }

    sprintf(temp_path, "/sys/class/pwm/%s/pwm%s/%s", pwmx.pwmchip, pwmx.channel, attr);
    fd = open(temp_path, O_WRONLY);
    if (fd < 0) 
    {
        printf("open %s fail!\n", temp_path);
        return -1;
    }

    len = strlen(val);
    if (len != write(fd, val, len)) 
    {
        printf("echo %s > %s fail!\n", val, temp_path);
        close(fd);
        return -1;
    }

    close(fd);  
    return 0;
}

/*****************************
 * @brief : pwm初始化
 * @param : pwmx, pwm结构体
 * @return: -1初始化失败 0初始化成功
*****************************/
static int pwm_init(pwm pwmx)
{   
    char temp_path[100];

    if (pwmx.pwmchip == NULL || pwmx.channel == NULL)
    {
        printf("%s : illegal input-parameters\n", __FUNCTION__);
        return -1;
    }

    sprintf(temp_path, "/sys/class/pwm/%s/pwm%s", pwmx.pwmchip, pwmx.channel);

    if (access(temp_path, F_OK)) 
    {
        int fd;

        sprintf(temp_path, "/sys/class/pwm/%s/export", pwmx.pwmchip);
        fd = open(temp_path, O_WRONLY);
        if (fd < 0)
        {
            printf("open %s fail!\n", temp_path);
            return -1;
        }

        if (write(fd, pwmx.channel, 1) != 1) 
        {
            printf("echo %s > %s fail!\n", pwmx.channel, temp_path);
            close(fd);
            return -1;
        }

        close(fd);
    }

    return 0;
}

/*****************************
 * @brief : pwm反初始化
 * @param : pwmx, pwm结构体
 * @return: -1反初始化失败 0反初始化成功
*****************************/
static int pwm_exit(pwm pwmx)
{
    char temp_path[100];

    if (pwmx.pwmchip == NULL || pwmx.channel == NULL)
    {
        printf("%s : illegal input-parameters\n", __FUNCTION__);
        return -1;
    }

    sprintf(temp_path, "/sys/class/pwm/%s/pwm%s", pwmx.pwmchip, pwmx.channel);

    if (access(temp_path, F_OK) == 0) 
    {
        int fd;

        sprintf(temp_path, "/sys/class/pwm/%s/unexport", pwmx.pwmchip);
        fd = open(temp_path, O_WRONLY);
        if (fd < 0)
        {
            printf("open %s fail!\n", temp_path);
            return -1;
        }

        if (write(fd, pwmx.channel, 1) != 1) 
        {
            printf("echo %s > %s fail!\n", pwmx.channel, temp_path);
            close(fd);
            return -1;
        }
        
        close(fd);
    }

    return 0;
}

static void sigint_handler(int sig_num) 
{    
    pwm_config(pwm1, "duty_cycle", "500000");       
    pwm_config(pwm2, "duty_cycle", "500000");

    pwm_exit(pwm1);
    pwm_exit(pwm2);
    exit(0);  
}

int main(int argc, char **argv)
{
    int ret;

    /* register exit signal ( Ctrl + c ) */
    signal(SIGINT, sigint_handler);

    /* init PWM1 */
    snprintf(pwm1.pwmchip, sizeof(pwm1.pwmchip), "%s", GPIOCHIP_PWM1);
    snprintf(pwm1.channel, sizeof(pwm1.channel), "%s", PWM1_CHANNEL);
    ret = pwm_init(pwm1);
    if(ret == -1)
    {
        printf("pwm1 init err!\n");
        return ret;
    }

    pwm_config(pwm1, "period", "20000000");             // 20ms
    pwm_config(pwm1, "duty_cycle", "500000");           // 0.5ms 0度
    pwm_config(pwm1, "polarity", "normal");
    pwm_config(pwm1, "enable", "1");

    /* inti PWM2 */
    snprintf(pwm2.pwmchip, sizeof(pwm2.pwmchip), "%s", GPIOCHIP_PWM2);
    snprintf(pwm2.channel, sizeof(pwm2.channel), "%s", PWM2_CHANNEL);
    ret = pwm_init(pwm2);
    if(ret == -1)
    {
        printf("pwm2 init err!\n");
        return ret;
    }

    pwm_config(pwm2, "period", "20000000");             // 20ms
    pwm_config(pwm2, "duty_cycle", "500000");           // 0.5ms 0度
    pwm_config(pwm2, "polarity", "normal");
    pwm_config(pwm2, "enable", "1");

    while(1)
    {
        pwm_config(pwm1, "duty_cycle", "500000");       // 0.5ms 0度
        pwm_config(pwm2, "duty_cycle", "500000");

        sleep(1);

        pwm_config(pwm1, "duty_cycle", "1000000");      // 1ms 45度
        pwm_config(pwm2, "duty_cycle", "1000000");

        sleep(1);

        pwm_config(pwm1, "duty_cycle", "1500000");      // 1.5ms 90度
        pwm_config(pwm2, "duty_cycle", "1500000");

        sleep(1);

        pwm_config(pwm1, "duty_cycle", "2000000");      // 2ms 135度
        pwm_config(pwm2, "duty_cycle", "2000000");

        sleep(1);

        pwm_config(pwm1, "duty_cycle", "2500000");      // 2.5ms 180度
        pwm_config(pwm2, "duty_cycle", "2500000");

        sleep(1);
    }

    return 0;
}
