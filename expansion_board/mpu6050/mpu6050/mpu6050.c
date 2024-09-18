/*
*
*   file: mpu6050.c
*   update: 2024-08-28
*   usage: 
*       make
*       sudo ./mpu6050
*
*/

#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include "i2cbusses.h"
#include <time.h>
#include <stdint.h>
#include <signal.h> 

uint8_t dev_addr = 0x68;
int i2c_bus = 5;
int file;
char filename[20];

void mpu6050_write_reg(uint8_t reg, uint8_t cmd)
{
    i2c_smbus_write_byte_data(file, reg, cmd);
}

uint8_t mpu6050_read_reg(uint8_t reg)
{
    return i2c_smbus_read_byte_data(file, reg);
}

int mpu6050_init(void)
{
    uint8_t id;

    file = open_i2c_dev(i2c_bus, filename, sizeof(filename), 0);
	if (file < 0)
	{
		printf("can't open %s\n", filename);
		return -1;
	}

	if (set_slave_addr(file, dev_addr, 1))
	{
		printf("can't set_slave_addr\n");
		return -1;
	}

    /* 读取id，检测i2c通信是否正常 */
    id = mpu6050_read_reg(0x75);
    if(id != 0x68)
    {
        printf("can not find mpu6050!\n");
        return -1;
    }

    /* 电源管理1寄存器：复位 */
    mpu6050_write_reg(0x6B, 0x80);
    usleep(100000); 

    /* 电源管理1寄存器：唤醒mpu6050，自动选择时钟源 */
    mpu6050_write_reg(0x6B, 0x01);

    /* 电源管理2寄存器：禁止加速度和陀螺仪进入待机 */
    mpu6050_write_reg(0x6C, 0x00);

    /* 中断使能寄存器：禁止所有中断 */
    mpu6050_write_reg(0x38, 0x00);

    /* 陀螺仪采样率分频寄存器：设置陀螺仪采样频率为1kHz */
    mpu6050_write_reg(0x19, 0x00);

    /* 配置寄存器：设置数字低通滤波带宽为20Hz */
    mpu6050_write_reg(0x1A, 0x04);

    /* 陀螺仪配置寄存器：设置陀螺仪不自检、角度量程配置为±2000°/s */
    mpu6050_write_reg(0x1B, 0x18);

    /* 加速度计配置寄存器：设置加速度计不自检、角度量程配置为±4g */
    mpu6050_write_reg(0x1C, 0x08);

    return 0;
}

void mpu6050_read_gyro(short int *buf)
{
    short int data;

    data = mpu6050_read_reg(0x43);
    data <<= 8;
    data |= mpu6050_read_reg(0x44);
    buf[0] = data;

    data = mpu6050_read_reg(0x45);
    data <<= 8;
    data |= mpu6050_read_reg(0x46);
    buf[1] = data;

    data = mpu6050_read_reg(0x47);
    data <<= 8;
    data |= mpu6050_read_reg(0x48);
    buf[2] = data;
}

void mpu6050_read_accel(short int *buf)
{
    short int data;

    data = mpu6050_read_reg(0x3B);
    data <<= 8;
    data |= mpu6050_read_reg(0x3C);
    buf[0] = data;

    data = mpu6050_read_reg(0x3D);
    data <<= 8;
    data |= mpu6050_read_reg(0x3E);
    buf[1] = data;

    data = mpu6050_read_reg(0x3F);
    data <<= 8;
    data |= mpu6050_read_reg(0x40);
    buf[2] = data;
}

void sigint_handler(int sig_num) 
{    
    
    exit(0);  
}

int main(int argc, char **argv)
{
    short int gyro_buf[3];
    short int accel_buf[3];
    int ret;

    float lsb_sensitivity_accel = 8192;     
    float lsb_sensitivity_gyro = 16.4;

    ret = mpu6050_init();
    if(ret < 0)
    {
        printf("mpu6050 init err!\n");
        return -1;
    }

    while(1)
    {   
        mpu6050_read_gyro(gyro_buf);
        mpu6050_read_accel(accel_buf);

        printf("Acceleration : x:%.2f, y:%.2f, z:%.2f (m/s^2)\n", accel_buf[0]/lsb_sensitivity_accel, accel_buf[1]/lsb_sensitivity_accel, accel_buf[2]/lsb_sensitivity_accel);
        printf("Gyro : x:%.2f, y:%.2f, z:%.2f (°/s)\n\n", gyro_buf[0]/lsb_sensitivity_gyro, gyro_buf[1]/lsb_sensitivity_gyro, gyro_buf[2]/lsb_sensitivity_gyro);
        
        usleep(50000);
    }

    return 0;
}