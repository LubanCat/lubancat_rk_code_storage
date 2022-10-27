#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
 
 
static char pwm_path[75];
static int pwm_config(const char *attr, const char *val)    //配置PWM
{
    char file_path[100];
    int len;
    int fd;
    sprintf(file_path, "%s/%s", pwm_path, attr);
    if ((fd = open(file_path, O_WRONLY))< 0) {
        perror("open error");
        return fd;
    }
    len = strlen(val);
    if (len != write(fd, val, len)) {
        perror("write error");
        close(fd);
        return -1;
    }
    close(fd); //关闭文件
    return 0;
}
 
 
int main(int argc, char *argv[])
{
    /* 校验传参 */
    if (4 != argc) {
        fprintf(stderr, "usage: %s <id> <period> <duty>\n",argv[0]);
        exit(-1);
    }

    /* 导出 pwm */
    sprintf(pwm_path, "/sys/class/pwm/pwmchip%s/pwm0", argv[1]);
    //如果 pwm0 目录不存在, 则导出
    if (access(pwm_path, F_OK)) {
        char temp[100];
        int fd;
        sprintf(temp, "/sys/class/pwm/pwmchip%s/export", argv[1]);
        if (0 > (fd = open(temp, O_WRONLY))) {
            perror("open error");
            exit(-1);
        }
        //导出 pwm
        if (1 != write(fd, "0", 1)) {
            perror("write error");
            close(fd);
            exit(-1);
        }
        close(fd); //关闭文件
    }
    
    /* 配置 PWM 周期 */
    if (pwm_config("period", argv[2]))
        exit(-1);
    /* 配置占空比 */
    if (pwm_config("duty_cycle", argv[3]))
        exit(-1);
    /* 使能 pwm */
    pwm_config("enable", "1");
    /* 退出程序 */
    exit(0);
}
