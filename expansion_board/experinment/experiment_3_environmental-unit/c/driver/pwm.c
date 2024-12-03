/*
*
*   file: pwm.c
*   update: 2024-08-15
*   function: 
*   
*/

#include "pwm.h"

static int flag_init = 0;

/*****************************
 * @brief : pwm属性配置
 * @param : pwmx, pwm结构体
 * @param : attr, pwm属性
 * @param : val,  pwm属性所要设置的值
 * @return: -1配置失败 0配置成功
*****************************/
int pwm_config(pwm pwmx, const char *attr, const char *val)
{
    char temp_path[100];
    int len;
    int fd;

    if(!flag_init)
        return -1;

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
int pwm_init(pwm pwmx)
{   
    char temp_path[100];

    if(flag_init)
        return -1;

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

    flag_init = 1;

    return 0;
}

/*****************************
 * @brief : pwm反初始化
 * @param : pwmx, pwm结构体
 * @return: -1反初始化失败 0反初始化成功
*****************************/
int pwm_exit(pwm pwmx)
{
    char temp_path[100];

    if(!flag_init)
        return -1;

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

    flag_init = 0;

    return 0;
}