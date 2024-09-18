/*
*
*   file: mpu6050.c
*   update: 2024-09-13
*
*/

#include "mpu6050.h"

static uint8_t dev_addr = 0x68;
static int file;
static char filename[20];

/*****************************
 * @brief : mpu6050写寄存器
 * @param : reg 寄存器地址
 * @param : cmd 要写入寄存器的值
 * @return: none
*****************************/
static void mpu6050_write_reg(uint8_t reg, uint8_t cmd)
{
    i2c_smbus_write_byte_data(file, reg, cmd);
}

/*****************************
 * @brief : mpu6050读寄存器
 * @param : reg 寄存器地址
 * @return: 返回读到的值
*****************************/
static uint8_t mpu6050_read_reg(uint8_t reg)
{
    return i2c_smbus_read_byte_data(file, reg);
}

/*****************************
 * @brief : mpu6050初始化
 * @param : i2c_bus i2c总线编号
 * @return: 0成功 -1失败
*****************************/
int mpu6050_init(uint8_t i2c_bus)
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

/*****************************
 * @brief : mpu6050读陀螺仪数据
 * @param : buf 用于保存读取到的陀螺仪数据
 * @return: none
*****************************/
void mpu6050_read_gyro(short int *buf)
{
    short int data;

    if(file < 0)
        return;

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

/*****************************
 * @brief : mpu6050读加速度数据
 * @param : buf 用于保存读取到的加速度数据
 * @return: none
*****************************/
void mpu6050_read_accel(short int *buf)
{
    short int data;

    if(file < 0)
        return;

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