/*
*
*   file: motor.c
*   update: 2024-08-09
*   function: 适用于野火电机驱动模块
*
*/

#include "motor.h"

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

/*****************************
 * @brief : 电机驱动板初始化
 * @param : AIN1 AIN2, 通道A正反转设置（AIN1=0 AIN2=1 正转）（AIN1=1 AIN2=0 反转）
 * @param : BIN1 BIN2, 通道B正反转设置（BIN1=0 BIN2=1 正转）（BIN1=1 BIN2=0 反转）
 * @return: -1初始化失败 0初始化成功
*****************************/
int motor_init(uint8_t AIN1, uint8_t AIN2, uint8_t BIN1, uint8_t BIN2)
{
    int ret;

    if((AIN1 != 0 && AIN1 != 1) || (AIN2 != 0 && AIN2 != 1) || (BIN1 != 0 && BIN1 != 1) || (BIN2 != 0 && BIN2 != 1))
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
    ret = gpiod_line_request_output(STBY_line, "STBY_line", 0);   
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

    /* init PWMA */
    snprintf(pwma.pwmchip, sizeof(pwma.pwmchip), "%s", GPIOCHIP_PWMA);
    snprintf(pwma.channel, sizeof(pwma.channel), "%s", "0");
    ret = pwm_init(pwma);
    if(ret == -1)
    {
        printf("pwma init err!\n");
        return ret;
    }

    /* inti PWMB */
    snprintf(pwmb.pwmchip, sizeof(pwmb.pwmchip), "%s", GPIOCHIP_PWMB);
    snprintf(pwmb.channel, sizeof(pwmb.channel), "%s", "0");
    ret = pwm_init(pwmb);
    if(ret == -1)
    {
        printf("pwmb init err!\n");
        return ret;
    }

    return 0;
}

/*****************************
 * @brief : 电机驱动板反初始化
 * @param : none
 * @return: -1反初始化失败 0反初始化成功
*****************************/
int motor_release(void) 
{    
    int ret = 0;

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

    ret = pwm_exit(pwma);
    if(ret == -1)
        printf("pwma exit fail!\n");

    ret = pwm_exit(pwmb);
    if(ret == -1)
        printf("pwmb exit fail!\n");

    return ret;
}

/*****************************
 * @brief : 电机驱动板使能
 * @param : none
 * @return: none
*****************************/
void motor_on(void)
{
    gpiod_line_set_value(STBY_line, 1);
}

/*****************************
 * @brief : 电机驱动板关闭
 * @param : none
 * @return: none
*****************************/
void motor_off(void)
{
    gpiod_line_set_value(STBY_line, 0);
}

/*****************************
 * @brief : 电机驱动板通道A正反转设置
 * @param : val1=0 val=1 or val1=1 val1=0
 * @return: none
*****************************/
void motor_channelA_direction_set(uint8_t val1, uint8_t val2)
{
    if((val1 != 0 && val1 != 1) || (val2 != 0 && val2 != 1))
        return;

    gpiod_line_set_value(AIN1_line, val1);
    gpiod_line_set_value(AIN2_line, val2);
}

/*****************************
 * @brief : 电机驱动板通道B正反转设置
 * @param : val1=0 val=1 or val1=1 val1=0
 * @return: none
*****************************/
void motor_channelB_direction_set(uint8_t val1, uint8_t val2)
{
    if((val1 != 0 && val1 != 1) || (val2 != 0 && val2 != 1))
        return;

    gpiod_line_set_value(BIN1_line, val1);
    gpiod_line_set_value(BIN2_line, val2);
}

/*****************************
 * @brief : 电机驱动板PWMA熟悉设置
 * @param : attr, pwm属性
 * @param : val,  pwm属性所要设置的值
 * @return: -1配置失败 0配置成功
*****************************/
int motor_pwmA_config(const char *attr, const char *val)
{
    return pwm_config(pwma, attr, val);
}

/*****************************
 * @brief : 电机驱动板PWMB熟悉设置
 * @param : attr, pwm属性
 * @param : val,  pwm属性所要设置的值
 * @return: -1配置失败 0配置成功
*****************************/
int motor_pwmB_config(const char *attr, const char *val)
{
    return pwm_config(pwmb, attr, val);
}