#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>

/*
 * ======================== 计算说明 ========================
 * 1. ADS1115电压计算公式：
 *    电压(V) = 原始值(raw) × 缩放系数(scale) / 1000
 *    （scale单位：mV/LSB，除以1000转为V）
 * 2. 支持4通道电压采集：AIN0 ~ AIN3
 */

/* 全局退出标志 */
static int exit_flag = 0;

/* Ctrl+C 信号处理 */
static void sigint_handler(int sig)
{
    if (sig == SIGINT) {
        exit_flag = 1;
        printf("\n程序退出中...\n");
    }
}

/* 读取通道原始值 in_voltageX_raw */
static int read_ads_raw(const char *iio_dev, int ch)
{
    char path[PATH_MAX];
    char buf[16];
    int fd, ret;

    // 拼接完整的 sysfs 文件路径
    snprintf(path, sizeof(path), "%s/in_voltage%d_raw", iio_dev, ch);
    
    // 以只读方式打开节点文件
    fd = open(path, O_RDONLY);
    if (fd < 0) return INT_MIN;

    ret = read(fd, buf, sizeof(buf)-1);
    close(fd);
    return (ret < 0) ? INT_MIN : atoi(buf);
}

/* 读取通道独立缩放系数 in_voltageX_scale */
static float read_ads_scale(const char *iio_dev, int ch)
{
    char path[PATH_MAX];
    char buf[32];
    int fd, ret;

    // 拼接完整的 sysfs 文件路径
    snprintf(path, sizeof(path), "%s/in_voltage%d_scale", iio_dev, ch);
    
    // 以只读方式打开节点文件
    fd = open(path, O_RDONLY);
    if (fd < 0) return -1.0f;

    ret = read(fd, buf, sizeof(buf)-1);
    close(fd);
    return (ret < 0) ? -1.0f : atof(buf);
}

int main(int argc, char *argv[])
{
    int raw[4];
    float scale[4];
    float volt[4];
    int i;

    signal(SIGINT, sigint_handler);

    if (argc != 2) {
        printf("使用方法: ./adc_ads1115_app /sys/bus/iio/devices/iio:device1\n");
        return -1;
    }

    printf("===== ADS1115 4通道独立读取程序 =====\n");
    printf("设备路径: %s\n", argv[1]);
    printf("按 Ctrl+C 退出\n");
    printf("-----------------------------------------\n");
    printf("CH0(V)\tCH1(V)\tCH2(V)\tCH3(V)\n");
    printf("-----------------------------------------\n");

    while (!exit_flag)
    {
        // 逐通道读取 raw 和 scale
        for (i = 0; i < 4; i++) {
            raw[i] = read_ads_raw(argv[1], i);
            scale[i] = read_ads_scale(argv[1], i);
        }

        // 数据校验
        int valid = 1;
        for (i = 0; i < 4; i++) {
            if (raw[i] == INT_MIN || scale[i] <= 0.0f)
                valid = 0;
        }

        if (!valid) {
            printf("读取节点失败，重试...\n");
            sleep(1);
            continue;
        }

        // 逐通道计算电压
        for (i = 0; i < 4; i++) {
            volt[i] = (float)raw[i] * scale[i] / 1000.0f;
        }

        // 打印结果
        printf("%.2f\t%.2f\t%.2f\t%.2f\n",
               volt[0], volt[1], volt[2], volt[3]);

        sleep(1);
    }

    return 0;
}