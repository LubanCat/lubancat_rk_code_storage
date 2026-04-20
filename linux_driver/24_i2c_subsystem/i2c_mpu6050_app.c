#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

/* 
 * 计算公式：
 * 加速度值（m/s²）= ACCEL_OUT / ACCEL_SCALE * GRAVITY
 * 陀螺仪值（deg/s）= GYRO_OUT / GYRO_SCALE
 * 温度(℃)= TEMP_OUT / TEMP_SCALE + TEMP_OFFSET
 */

/* 定义单次读取的字节数：7个short类型 = 14字节（加速度6 + 温度2 + 陀螺仪6） */
#define READ_BYTES 14

/* 量程转换系数，与驱动中配置一致 */
#define ACCEL_SCALE 16384.0    // 加速度计±2G量程：16384 LSB/g
#define GYRO_SCALE  16.4       // 陀螺仪±2000deg/s量程：16.4 LSB/(deg/s)
#define TEMP_SCALE  340.0      // 温度传感器灵敏度：340 LSB/°C
#define TEMP_OFFSET 36.53      // 温度传感器零点偏移：36.53°C
#define GRAVITY     9.8        // 重力加速度（m/s²）

/* 全局标志，用于Ctrl+C退出循环读取 */
static int exit_flag = 0;

/* 信号处理函数：捕获Ctrl+C，设置退出标志 */
static void sigint_handler(int sig)
{
    if (sig == SIGINT)
    {
        exit_flag = 1;
        printf("\n收到退出信号，即将停止读取...\n");
    }
}

int main(int argc, char *argv[])
{
    /* 保存收到的mpu6050转换结果数据，依次为：
     * AX(加速度X), AY, AZ, TEMP(温度), GX(陀螺仪X), GY, GZ 
     */
    short receive_data[7] = {0};
    
    /* 物理值变量 */
    float ax_ms2, ay_ms2, az_ms2;    // 加速度 (m/s²)
    float temp_c;                    // 温度 (°C)
    float gx_dps, gy_dps, gz_dps;    // 陀螺仪 (deg/s)
    
    int fd = -1;
    int ret = -1;
    
    // 注册Ctrl+C信号处理函数
    signal(SIGINT, sigint_handler);
    
    // 检查参数，需传入设备节点路径
    if (argc != 2) {
        printf("Usage: ./i2c_mpu6050_app /dev/i2c_mpu6050\n");
        return -1;
    }
    
    /* 打开文件 */
    fd = open(argv[1], O_RDWR);
    if(fd < 0)
    {
        printf("打开设备文件 %s 失败 !\n", argv[1]);
        return -1;
    }
    
    printf("设备文件 %s 打开成功，开始持续读取数据（Ctrl+C退出）...\n", argv[1]);
    
    /* 打印物理单位表头 */
    printf("AX(m/s²)   AY(m/s²)   AZ(m/s²)   TEMP(°C)   GX(°/s)    GY(°/s)    GZ(°/s)\n");
    printf("----------------------------------------------------------------------------------------\n");
    
    /* 循环读取数据，直到捕获Ctrl+C */
    while (!exit_flag)
    {
        /* 复位接收缓冲区，避免脏数据 */
        memset(receive_data, 0, sizeof(receive_data));
        
        /* 读取数据，严格读取14字节 */
        ret = read(fd, receive_data, READ_BYTES);
        if (ret < 0)
        {
            printf("读取设备文件数据失败 !\n");
            close(fd);
            return -1;
        }
        else if (ret != READ_BYTES)
        {
            /* 处理短读/中断异常，不退出，继续重试 */
            printf("读取字节数不匹配，预期%d字节，实际%d字节，重试...\n", READ_BYTES, ret);
            usleep(100000);
            continue;
        }
        
        /* ============ 原始数据转物理值 ============ */
        
        /* 加速度转换：加速度值（m/s²）= ACCEL_OUT / ACCEL_SCALE * GRAVITY */
        ax_ms2 = (float)receive_data[0] / ACCEL_SCALE * GRAVITY;
        ay_ms2 = (float)receive_data[1] / ACCEL_SCALE * GRAVITY;
        az_ms2 = (float)receive_data[2] / ACCEL_SCALE * GRAVITY;
        
        /* 温度转换：温度(℃)= TEMP_OUT / TEMP_SCALE + TEMP_OFFSET */
        temp_c = (float)receive_data[3] / TEMP_SCALE + TEMP_OFFSET;
        
        /* 陀螺仪转换：陀螺仪值（deg/s）= GYRO_OUT / GYRO_SCALE */
        gx_dps = (float)receive_data[4] / GYRO_SCALE;
        gy_dps = (float)receive_data[5] / GYRO_SCALE;
        gz_dps = (float)receive_data[6] / GYRO_SCALE;
        
        /* 打印物理值，格式对齐，保留2位小数 */
        printf("%-10.2f %-10.2f %-10.2f %-10.2f %-10.2f %-10.2f %-10.2f\n",
               ax_ms2, ay_ms2, az_ms2,
               temp_c,
               gx_dps, gy_dps, gz_dps);
        
        /* 间隔1s时间再读取 */
        sleep(1);
    }
    
    /* 关闭文件 */
    ret = close(fd);
    if(ret < 0)
    {
        printf("关闭设备文件 %s 失败 !\n", argv[1]);
        return -1;
    }
    
    return 0;
}