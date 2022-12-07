#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

/*寄存器地址*/
#define SMPLRT_DIV      0x19
#define PWR_MGMT_1      0x6B
#define CONFIG          0x1A
#define ACCEL_CONFIG    0x1C

#define ACCEL_XOUT_H    0x3B
#define ACCEL_XOUT_L    0x3C
#define ACCEL_YOUT_H    0x3D
#define ACCEL_YOUT_L    0x3E
#define ACCEL_ZOUT_H    0x3F
#define ACCEL_ZOUT_L    0x40
#define GYRO_XOUT_H     0x43
#define GYRO_XOUT_L     0x44
#define GYRO_YOUT_H     0x45
#define GYRO_YOUT_L     0x46
#define GYRO_ZOUT_H     0x47
#define GYRO_ZOUT_L     0x48

//从机地址 MPU6050地址
#define Address         0x68

//MPU6050操作相关函数
static int mpu6050_init(int fd,uint8_t addr);
static int i2c_write(int fd, uint8_t addr,uint8_t reg,uint8_t val);
static int i2c_read(int fd, uint8_t addr,uint8_t reg,uint8_t * val);
static short GetData(int fd,uint8_t addr,unsigned char REG_Address);

int main(int argc,char *argv[] )
{
    int  fd;
    fd = I2C_SLAVE;

    if(argc < 2){
        printf("Wrong use !!!!\n");
        printf("Usage: %s [dev]\n",argv[0]);
        return -1;
    }

    fd = open(argv[1], O_RDWR); // open file and enable read and  write
    if (fd < 0){
        printf("Can't open %s \n",argv[1]); // open i2c dev file fail
        exit(1);
    }

    //初始化MPU6050
    mpu6050_init(fd,Address);
    while(1){
        usleep(1000 * 10);
        printf("ACCE_X:%6d\n ", GetData(fd,Address,ACCEL_XOUT_H));
        usleep(1000 * 10);
        printf("ACCE_Y:%6d\n ", GetData(fd,Address,ACCEL_YOUT_H));
        usleep(1000 * 10);
        printf("ACCE_Z:%6d\n ", GetData(fd,Address,ACCEL_ZOUT_H));
        usleep(1000 * 10);
        printf("GYRO_X:%6d\n ", GetData(fd,Address,GYRO_XOUT_H));
        usleep(1000 * 10);
        printf("GYRO_Y:%6d\n ", GetData(fd,Address,GYRO_YOUT_H));
        usleep(1000 * 10);
        printf("GYRO_Z:%6d\n\n ", GetData(fd,Address,GYRO_ZOUT_H));
        sleep(1);
    }

    close(fd);

    return 0;
}

static int mpu6050_init(int fd,uint8_t addr)
{
    i2c_write(fd, addr,PWR_MGMT_1,0x00);  //配置电源管理，0x00,正常启动
    i2c_write(fd, addr,SMPLRT_DIV,0x07);  //设置MPU6050的输出分频既设置采样
    i2c_write(fd, addr,CONFIG,0x06);  //配置数字低通滤波器和帧同步引脚
    i2c_write(fd, addr,ACCEL_CONFIG,0x01);  //设置量程和 X、Y、Z 轴加速度自检

    return 0;
}

static int i2c_write(int fd, uint8_t addr,uint8_t reg,uint8_t val)
{
    int retries;
    uint8_t data[2];

    data[0] = reg;
    data[1] = val;

    //设置地址长度：0为7位地址
    ioctl(fd,I2C_TENBIT,0);

    //设置从机地址
    if (ioctl(fd,I2C_SLAVE,addr) < 0){
        printf("fail to set i2c device slave address!\n");
        close(fd);
        return -1;
    }

    //设置收不到ACK时的重试次数
    ioctl(fd,I2C_RETRIES,5);

    if (write(fd, data, 2) == 2){
        return 0;
    }
    else{
        return -1;
    }

}

static int i2c_read(int fd, uint8_t addr,uint8_t reg,uint8_t * val)
{
    int retries;

    //设置地址长度：0为7位地址
    ioctl(fd,I2C_TENBIT,0);

    //设置从机地址
    if (ioctl(fd,I2C_SLAVE,addr) < 0){
        printf("fail to set i2c device slave address!\n");
        close(fd);
        return -1;
    }

    //设置收不到ACK时的重试次数
    ioctl(fd,I2C_RETRIES,5);

    if (write(fd, &reg, 1) == 1){
        if (read(fd, val, 1) == 1){
                return 0;
        }
    }
    else{
        return -1;
    }
}

static short GetData(int fd,uint8_t addr,unsigned char REG_Address)
{
    char H, L;

    i2c_read(fd, addr,REG_Address, &H);
    usleep(1000);
    i2c_read(fd, addr,REG_Address + 1, &L);
    return (H << 8) +L;
}