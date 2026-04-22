#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>

/*
 *  ======================== 计算说明 ========================
 * 1. 加速度(m/s²) = 原始值 * 加速度scale * 重力加速度(9.8)
 * 2. 温度(℃)     = 原始值 * 温度scale + 36.53
 * 3. 陀螺仪(°/s)  = 原始值 * 陀螺仪scale
 * 
 */

/* 物理常量 */
#define TEMP_OFFSET 36.53f     // 温度传感器零点偏移：36.53°C
#define TEMP_DIV    340.0f     // 温度传感器灵敏度：340 LSB/°C
#define GRAVITY     9.8f       // 标准重力加速度

/* 全局标志，用于Ctrl+C退出循环读取 */
static int exit_flag = 0;

/**
 * @brief  Ctrl+C信号处理函数
 * @param  sig: 信号编号
 */
static void sigint_handler(int sig)
{
    if (sig == SIGINT)
    {
        exit_flag = 1;
        printf("\n收到退出信号，即将停止读取...\n");
    }
}

/**
 * @brief  读取IIO原始整型数据(raw节点)
 * @param  iio_path: IIO设备路径
 * @param  name: 节点文件名
 * @return 成功返回整型数据，失败返回INT_MIN
 */
static int read_iio_raw(const char *iio_path, const char *name)
{
    char file_path[PATH_MAX];  // 存储拼接后的完整文件路径
    char buf[16] = {0};        // 存储读取到的字符串数据
    int fd, ret;               // 文件描述符、函数返回值

    // 拼接完整的 sysfs 文件路径
    snprintf(file_path, sizeof(file_path), "%s/%s", iio_path, name);
    fd = open(file_path, O_RDONLY);
    if (fd < 0) return INT_MIN;

    // 以只读方式打开节点文件
    ret = read(fd, buf, sizeof(buf)-1);
    close(fd);
    if (ret < 0) return INT_MIN;    // 读取数据失败，返回错误标识

    return atoi(buf);   // 将字符串转为整型，返回原始数据
}

/**
 * @brief  读取IIO浮点缩放系数(scale节点)
 * @param  iio_path: IIO设备路径
 * @param  name: scale文件名
 * @return 成功返回浮点系数，失败返回-1.0f
 */
static float read_iio_scale(const char *iio_path, const char *name)
{
    char file_path[PATH_MAX];   // 存储拼接后的完整文件路径
    char buf[32] = {0};         // 存储读取到的字符串数据
    int fd, ret;                // 文件描述符、函数返回值

    // 拼接完整的 sysfs 文件路径
    snprintf(file_path, sizeof(file_path), "%s/%s", iio_path, name);
    
    // 以只读方式打开节点文件
    fd = open(file_path, O_RDONLY);
    if (fd < 0) return -1.0f;   // 失败返回-1.0f

    ret = read(fd, buf, sizeof(buf)-1);
    close(fd);
    if (ret < 0) return -1.0f;  // 失败返回-1.0f

    return atof(buf);   // 将字符串转为整型，返回原始数据
}

int main(int argc, char *argv[])
{
    // 传感器原始数据
    int accel_x, accel_y, accel_z;
    int temp_raw;
    int gyro_x, gyro_y, gyro_z;

    // 读取的缩放系数
    float accel_scale;
    float temp_scale;
    float gyro_scale;

    // 最终物理量
    float ax_ms2, ay_ms2, az_ms2;
    float temp_c;
    float gx_dps, gy_dps, gz_dps;

    // 注册信号
    signal(SIGINT, sigint_handler);

    // 参数校验
    if (argc != 2) {
        printf("使用方法: ./iio_mpu6050_app /sys/bus/iio/devices/iio:device1\n");
        return -1;
    }

    // 启动提示
    printf("===== MPU6050 IIO 数据读取程序 =====\n");
    printf("设备路径: %s\n", argv[1]);
    printf("按 Ctrl+C 退出程序\n");
    printf("===================================================\n");

    // 打印表头
    printf("AX(m/s²)   AY(m/s²)   AZ(m/s²)   TEMP(°C)   GX(°/s)    GY(°/s)    GZ(°/s)\n");
    printf("---------------------------------------------------------------------------\n");

    // 循环读取
    while (!exit_flag)
    {
        // 1. 读取加速度原始值
        accel_x = read_iio_raw(argv[1], "in_accel_x_raw");
        accel_y = read_iio_raw(argv[1], "in_accel_y_raw");
        accel_z = read_iio_raw(argv[1], "in_accel_z_raw");

        // 2. 读取温度原始值
        temp_raw = read_iio_raw(argv[1], "in_temp_raw");

        // 3. 读取陀螺仪原始值
        gyro_x = read_iio_raw(argv[1], "in_anglvel_x_raw");
        gyro_y = read_iio_raw(argv[1], "in_anglvel_y_raw");
        gyro_z = read_iio_raw(argv[1], "in_anglvel_z_raw");

        // 4. 读取scale（加速度/陀螺仪）
        accel_scale = read_iio_scale(argv[1], "in_accel_x_scale");
        temp_scale = read_iio_scale(argv[1], "in_temp_scale");
        gyro_scale  = read_iio_scale(argv[1], "in_anglvel_x_scale");

        // 数据校验
        if (accel_x == INT_MIN || accel_y == INT_MIN || accel_z == INT_MIN ||
            temp_raw == INT_MIN || gyro_x == INT_MIN || gyro_y == INT_MIN || gyro_z == INT_MIN ||
            accel_scale <= 0.0f || gyro_scale <= 0.0f)
        {
            sleep(1);
            continue;
        }

        // ===================== 原始数据转物理值 =====================
        // 加速度：通过scale计算
        ax_ms2 = (float)accel_x * accel_scale * GRAVITY;
        ay_ms2 = (float)accel_y * accel_scale * GRAVITY;
        az_ms2 = (float)accel_z * accel_scale * GRAVITY;

        // 温度：MPU6050硬件标准公式
        temp_c = (float)temp_raw * temp_scale + TEMP_OFFSET;

        // 陀螺仪：通过scale计算
        gx_dps = (float)gyro_x * gyro_scale;
        gy_dps = (float)gyro_y * gyro_scale;
        gz_dps = (float)gyro_z * gyro_scale;

        // 打印结果
        printf("%-10.2f %-10.2f %-10.2f %-10.2f %-10.2f %-10.2f %-10.2f\n",
               ax_ms2, ay_ms2, az_ms2,
               temp_c,
               gx_dps, gy_dps, gz_dps);

        sleep(1);
    }

    printf("程序已退出！\n");
    return 0;
}