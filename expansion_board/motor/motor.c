/*
*
*   file: motor.c
*   update: 2024-08-07
*   usage: 
*       sudo gcc -o motor motor.c -lgpiod
*       sudo ./motor
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

/* gpiochip */
#define GPIOCHIP_MOTORCTRL_STBY     "/dev/gpiochip1"
#define GPIOCHIP_MOTORCTRL_AIN1     "/dev/gpiochip4"
#define GPIOCHIP_MOTORCTRL_AIN2     "/dev/gpiochip3"
#define GPIOCHIP_MOTORCTRL_BIN1     "/dev/gpiochip1"
#define GPIOCHIP_MOTORCTRL_BIN2     "/dev/gpiochip1"

/* gpionum  
 *
 * A-D : 0-4
 * number = group * 8 + x
 * e.g. : B0 = 1 * 8 + 0 = 8
 *	      C4 = 2 * 8 + 4 = 20 
 */
#define GPIONUM_MOTORCTRL_STBY      (8)
#define GPIONUM_MOTORCTRL_AIN1      (20)
#define GPIONUM_MOTORCTRL_AIN2      (8)
#define GPIONUM_MOTORCTRL_BIN1      (9)
#define GPIONUM_MOTORCTRL_BIN2      (10)

struct gpiod_chip *STBY_gpiochip;
struct gpiod_chip *AIN1_gpiochip;   
struct gpiod_chip *AIN2_gpiochip;
struct gpiod_chip *BIN1_gpiochip;
struct gpiod_chip *BIN2_gpiochip;     

struct gpiod_line *STBY_line;
struct gpiod_line *AIN1_line;   
struct gpiod_line *AIN2_line;
struct gpiod_line *BIN1_line;
struct gpiod_line *BIN2_line;

typedef struct pwm {
    char pwmchip[10];
    char channel[10];
}pwm;
pwm pwm1, pwm2;

/*****************************
 * @brief : 电机驱动板初始化
 * @param : STBY, 0关闭 1使能
 * @param : AIN1 AIN2, （AIN1=0 AIN2=1 正转）（AIN1=1 AIN2=0 反转）
 * @param : BIN1 BIN2, （BIN1=0 BIN2=1 正转）（BIN1=1 BIN2=0 反转）
 * @return: -1初始化失败 0初始化成功
*****************************/
static int motor_controller_init(uint8_t STBY, uint8_t AIN1, uint8_t AIN2, uint8_t BIN1, uint8_t BIN2)
{
    int ret;

    if((STBY != 0 && STBY != 1) || (AIN1 != 0 && AIN1 != 1) || (AIN2 != 0 && AIN2 != 1) || (BIN1 != 0 && BIN1 != 1) || (BIN2 != 0 && BIN2 != 1))
    {
        printf("%s : illegal input-parameters\n", __FUNCTION__);
        return -1;
    }

    /* get gpio controller */
    STBY_gpiochip = gpiod_chip_open(GPIOCHIP_MOTORCTRL_STBY);  
    if(STBY_gpiochip == NULL)
    {
        printf("gpiod_chip_open error : STBY_gpiochip\n");
        return -1;
    }

    AIN1_gpiochip = gpiod_chip_open(GPIOCHIP_MOTORCTRL_AIN1);  
    if(AIN1_gpiochip == NULL)
    {
        printf("gpiod_chip_open error : AIN1_gpiochip\n");
        return -1;
    }

    AIN2_gpiochip = gpiod_chip_open(GPIOCHIP_MOTORCTRL_AIN2);  
    if(AIN2_gpiochip == NULL)
    {
        printf("gpiod_chip_open error : AIN2_gpiochip\n");
        return -1;
    }

    BIN1_gpiochip = gpiod_chip_open(GPIOCHIP_MOTORCTRL_BIN1);  
    if(BIN1_gpiochip == NULL)
    {
        printf("gpiod_chip_open error : BIN1_gpiochip\n");
        return -1;
    }

    BIN2_gpiochip = gpiod_chip_open(GPIOCHIP_MOTORCTRL_BIN2);  
    if(BIN2_gpiochip == NULL)
    {
        printf("gpiod_chip_open error : BIN2_gpiochip\n");
        return -1;
    }

    /* get gpio line */
    STBY_line = gpiod_chip_get_line(STBY_gpiochip, GPIONUM_MOTORCTRL_STBY);
    if(STBY_line == NULL)
    {
        printf("gpiod_chip_get_line error : GPIONUM_MOTORCTRL_STBY\n");
        return -1;
    }

    AIN1_line = gpiod_chip_get_line(AIN1_gpiochip, GPIONUM_MOTORCTRL_AIN1);
    if(AIN1_line == NULL)
    {
        printf("gpiod_chip_get_line error : GPIONUM_MOTORCTRL_AIN1\n");
        return -1;
    }

    AIN2_line = gpiod_chip_get_line(AIN2_gpiochip, GPIONUM_MOTORCTRL_AIN2);
    if(AIN2_line == NULL)
    {
        printf("gpiod_chip_get_line error : GPIONUM_MOTORCTRL_AIN2\n");
        return -1;
    }

    BIN1_line = gpiod_chip_get_line(BIN1_gpiochip, GPIONUM_MOTORCTRL_BIN1);
    if(BIN1_line == NULL)
    {
        printf("gpiod_chip_get_line error : GPIONUM_MOTORCTRL_BIN1\n");
        return -1;
    }

    BIN2_line = gpiod_chip_get_line(BIN2_gpiochip, GPIONUM_MOTORCTRL_BIN2);
    if(BIN2_line == NULL)
    {
        printf("gpiod_chip_get_line error : GPIONUM_MOTORCTRL_BIN2\n");
        return -1;
    }

    /* set the line direction to output mode, and initialize the level */
    ret = gpiod_line_request_output(STBY_line, "STBY_line", STBY);   
    if(ret < 0)
    {
        printf("gpiod_line_request_output error : STBY_line\n");
        return -1;
    }

    ret = gpiod_line_request_output(AIN1_line, "AIN1_line", AIN1);   
    if(ret < 0)
    {
        printf("gpiod_line_request_output error : AIN1_line\n");
        return -1;
    }

    ret = gpiod_line_request_output(AIN2_line, "AIN2_line", AIN2);   
    if(ret < 0)
    {
        printf("gpiod_line_request_output error : AIN2_line\n");
        return -1;
    }

    ret = gpiod_line_request_output(BIN1_line, "BIN1_line", BIN1);   
    if(ret < 0)
    {
        printf("gpiod_line_request_output error : BIN1_line\n");
        return -1;
    }

    ret = gpiod_line_request_output(BIN2_line, "BIN2_line", BIN2);   
    if(ret < 0)
    {
        printf("gpiod_line_request_output error : BIN2_line\n");
        return -1;
    }

    gpiod_line_set_value(STBY_line, STBY);
    gpiod_line_set_value(AIN2_line, AIN1);
    gpiod_line_set_value(AIN2_line, AIN2);
    gpiod_line_set_value(BIN2_line, BIN1);
    gpiod_line_set_value(BIN2_line, BIN2);

    return 0;
}

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

    pwm_config(pwmx, "polarity", "normal");
    pwm_config(pwmx, "period", "1000");
    pwm_config(pwmx, "duty_cycle", "600");
    pwm_config(pwmx, "enable", "1");

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
    /* motor controller off */
    gpiod_line_set_value(STBY_line, 0);

    /* release line */
    gpiod_line_release(STBY_line);
    gpiod_line_release(AIN1_line);
    gpiod_line_release(AIN2_line);
    gpiod_line_release(BIN1_line);
    gpiod_line_release(BIN2_line);

    gpiod_chip_close(STBY_gpiochip);
    gpiod_chip_close(AIN1_gpiochip);
    gpiod_chip_close(AIN2_gpiochip);
    gpiod_chip_close(BIN1_gpiochip);
    gpiod_chip_close(BIN2_gpiochip);

    pwm_exit(pwm1);
    pwm_exit(pwm2);

    exit(0);  
}

int main(int argc, char **argv)
{
    int ret;

    /* register exit signal ( Ctrl + c ) */
    signal(SIGINT, sigint_handler);

    /* motor controller init */
    ret = motor_controller_init(0, 0, 1, 0, 1);
    if(ret == -1)
    {
        printf("motor controller init err!\n");
        return ret;
    }

    /* init pwm1 */
    snprintf(pwm1.pwmchip, sizeof(pwm1.pwmchip), "%s", "pwmchip1");
    snprintf(pwm1.channel, sizeof(pwm1.channel), "%s", "0");
    ret = pwm_init(pwm1);
    if(ret == -1)
    {
        printf("pwm1 init err!\n");
        return ret;
    }

    /* inti pwm2 */
    snprintf(pwm2.pwmchip, sizeof(pwm2.pwmchip), "%s", "pwmchip2");
    snprintf(pwm2.channel, sizeof(pwm2.channel), "%s", "0");
    ret = pwm_init(pwm2);
    if(ret == -1)
    {
        printf("pwm2 init err!\n");
        return ret;
    }

    /* enable motor controller */
    gpiod_line_set_value(STBY_line, 1);

    while(1)
    {   

        gpiod_line_set_value(AIN1_line, 0);
        gpiod_line_set_value(AIN2_line, 1);
        gpiod_line_set_value(BIN1_line, 0);
        gpiod_line_set_value(BIN2_line, 1);
         
        sleep(2);

        gpiod_line_set_value(AIN1_line, 1);
        gpiod_line_set_value(AIN2_line, 0);
        gpiod_line_set_value(BIN1_line, 1);
        gpiod_line_set_value(BIN2_line, 0);

        sleep(2);
    }

    return 0;
}