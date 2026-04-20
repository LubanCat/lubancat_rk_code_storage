#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

/* PWM控制参数结构体，与内核驱动一致 */
typedef struct pwm_config_struct {
    unsigned int period;      /* PWM周期，单位：ns */
    unsigned int duty_cycle;  /* PWM占空比，单位：ns */
    unsigned int polarity;    /* 极性：0-正常 1-反相 */
    unsigned int enable;      /* 使能：0-关闭 1-开启 */
} pwm_config_struct;

int main(int argc, char *argv[])
{
    int fd;
    ssize_t write_ret;

    /* 初始化默认PWM配置，无参数时使用 */
    pwm_config_struct cfg = {
        .period      = 10000,   /* 默认周期：10000ns */
        .duty_cycle  = 5000,    /* 默认占空比：5000ns */
        .polarity    = 0,       /* 默认极性：正常 */
        .enable      = 1        /* 默认使能：开启 */
    };

    /* 参数校验：至少传入设备节点路径 */
    if (argc < 2) {
        printf("Usage: %s /dev/设备节点 [周期(ns)] [占空比(ns)] [极性] [使能]\n", argv[0]);
        printf("参数说明：\n");
        printf("  设备节点    : 必须输入\n");
        printf("  周期(ns)    : 可选，默认 10000\n");
        printf("  占空比(ns)  : 可选，默认 5000\n");
        printf("  极性        : 可选，0=正常 1=反相，默认 0\n");
        printf("  使能        : 可选，0=关闭 1=开启，默认 1\n");
        printf("示例1(默认)： %s /dev/pwm_subsystem\n", argv[0]);
        printf("示例2(自定义)：%s /dev/pwm_subsystem 20000 10000 0 1\n", argv[0]);
        return -1;
    }

    /* 根据传入的参数数量，覆盖默认配置 */
    if (argc >= 3)  cfg.period     = atoi(argv[2]);  // 第3个参数：周期
    if (argc >= 4)  cfg.duty_cycle = atoi(argv[3]);  // 第4个参数：占空比
    if (argc >= 5)  cfg.polarity   = atoi(argv[4]);  // 第5个参数：极性
    if (argc >= 6)  cfg.enable     = atoi(argv[5]);  // 第6个参数：使能

    /* 打开设备 */
    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        perror("open device failed");
        return -1;
    }

    /* 写入配置到内核驱动 */
    write_ret = write(fd, &cfg, sizeof(cfg));
    if (write_ret != sizeof(cfg))
    {
        perror("write pwm config failed");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}