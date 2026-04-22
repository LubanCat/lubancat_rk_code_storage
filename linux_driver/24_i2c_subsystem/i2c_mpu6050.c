#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>

/* MPU6050 寄存器地址定义 */
#define SMPLRT_DIV               0x19    /* 采样率分频寄存器 */
#define CONFIG                   0x1A    /* 通用配置寄存器(低通滤波/外部同步) */
#define GYRO_CONFIG              0x1B    /* 陀螺仪配置寄存器(量程设置) */
#define ACCEL_CONFIG             0x1C    /* 加速度计配置寄存器(量程设置) */
#define ACCEL_XOUT_H             0x3B    /* 加速度计X轴数据高8位 */
#define ACCEL_XOUT_L             0x3C    /* 加速度计X轴数据低8位 */
#define ACCEL_YOUT_H             0x3D    /* 加速度计Y轴数据高8位 */
#define ACCEL_YOUT_L             0x3E    /* 加速度计Y轴数据低8位 */
#define ACCEL_ZOUT_H             0x3F    /* 加速度计Z轴数据高8位 */
#define ACCEL_ZOUT_L             0x40    /* 加速度计Z轴数据低8位 */
#define TEMP_OUT_H               0x41    /* 温度传感器数据高8位 */
#define TEMP_OUT_L               0x42    /* 温度传感器数据低8位 */
#define GYRO_XOUT_H              0x43    /* 陀螺仪X轴数据高8位 */
#define GYRO_XOUT_L              0x44    /* 陀螺仪X轴数据低8位 */
#define GYRO_YOUT_H              0x45    /* 陀螺仪Y轴数据高8位 */
#define GYRO_YOUT_L              0x46    /* 陀螺仪Y轴数据低8位 */
#define GYRO_ZOUT_H              0x47    /* 陀螺仪Z轴数据高8位 */
#define GYRO_ZOUT_L              0x48    /* 陀螺仪Z轴数据低8位 */
#define PWR_MGMT_1               0x6B    /* 电源管理寄存器1(唤醒/时钟源) */
#define WHO_AM_I                 0x75    /* 器件ID寄存器(默认值0x68) */
#define SlaveAddress             0xD0    /* MPU6050 I2C 写地址(8位) */
#define Address                  0x68    /* MPU6050 I2C 7位从设备地址 */

/* 定义字符设备的名称 */
#define DEV_NAME    "i2c_mpu6050"
/* 定义字符设备的数量 */
#define DEV_CNT     1

/* 定义 mpu6050 设备结构体 */
struct mpu6050_device {
    /* 定义设备号变量 */
    dev_t devno;
    /* 定义设备类指针 */
    struct class *class;
    /* 定义设备指针 */
    struct device *device;
    /* 字符设备结构体 */
    struct cdev dev;
    /* I2C 客户端指针 */
    struct i2c_client *client;
};

/* 单设备驱动，静态定义的实体变量 */
static struct mpu6050_device mpu6050_dev;

/* 通过i2c 向mpu6050写入数据
 * mpu6050_client：mpu6050的i2c_client结构体。
 * address, 数据要写入的地址，
 * data, 要写入的数据
 * 返回值，错误，-1。成功，0  
 */
static int i2c_write_mpu6050(struct i2c_client *mpu6050_client, u8 address, u8 data)
{
    int ret = 0;

    /* 定义要发送的数据数组 */
    u8 write_data[2];
    
    /* 要发送的数据结构体 */
    struct i2c_msg mpu6050_write_msg;

    /* 设置要发送的数据 */
    write_data[0] = address;
    write_data[1] = data;

    /* 发送 iic 要写入的地址 reg */
    mpu6050_write_msg.addr = mpu6050_client->addr; //mpu6050在iic总线上的地址
    mpu6050_write_msg.flags = 0;                   //标记为发送数据
    mpu6050_write_msg.buf = write_data;            //数据的首地址
    mpu6050_write_msg.len = 2;                     //reg长度

    /* 执行发送 */
    ret = i2c_transfer(mpu6050_client->adapter, &mpu6050_write_msg, 1);
    if (ret != 1)
    {
        printk(KERN_ERR "i2c write mpu6050 error\n");
        return -1;
    }
    return 0;
}

/* 通过i2c 向mpu6050写入数据
 * mpu6050_client：mpu6050的i2c_client结构体。
 * address, 要读取的地址，
 * data，保存读取得到的数据
 * length，读长度
 * 返回值，错误，-1。成功，返回实际读取的字节数
 */
static int i2c_read_mpu6050(struct i2c_client *mpu6050_client, u8 address, void *data, u32 length)
{
    int ret = 0;

    /* 待读取的寄存器地址 */
    u8 address_data = address;

    /* I2C消息结构体数组 */
    struct i2c_msg mpu6050_read_msg[2];

    /*设置读取位置msg */
    mpu6050_read_msg[0].addr = mpu6050_client->addr; //mpu6050在 iic 总线上的地址
    mpu6050_read_msg[0].flags = 0;                   //标记为发送数据
    mpu6050_read_msg[0].buf = &address_data;         //写入的首地址
    mpu6050_read_msg[0].len = 1;                     //写入长度

    /* 设置读取位置msg */
    mpu6050_read_msg[1].addr = mpu6050_client->addr; //mpu6050在 iic 总线上的地址
    mpu6050_read_msg[1].flags = I2C_M_RD;            //标记为读取数据
    mpu6050_read_msg[1].buf = data;                  //读取得到的数据保存位置
    mpu6050_read_msg[1].len = length;                //读取长度

    /* 执行发送 */
    ret = i2c_transfer(mpu6050_client->adapter, mpu6050_read_msg, 2);

    if (ret != 2)
    {
        printk(KERN_ERR "i2c read mpu6050 error\n");
        return -1;
    }

    /* 成功返回实际读取的字节数 */
    return length;
}

/* 初始化mpu6050
 * client: I2C客户端指针
 * 返回值，成功，返回0。失败，返回 -1
 */
static int mpu6050_init(struct i2c_client *client)
{
    int ret = 0;
    u8 who_am_i = 0;

    /* 先读取器件ID寄存器进行检验 */
    ret = i2c_read_mpu6050(client, WHO_AM_I, &who_am_i, 1);
    if (ret < 0 || who_am_i != 0x68) {
        dev_err(&client->dev, "WHO_AM_I check failed! Read: 0x%x\n", who_am_i);
        return -ENODEV;
    }

    /* 配置电源管理寄存器：唤醒MPU6050，使用内部8Mhz时钟源 */
    ret = i2c_write_mpu6050(client, PWR_MGMT_1, 0x00);
    if (ret < 0) goto init_fail;
    
    /* 配置采样率分频寄存器：陀螺仪采样率，1KHz */
    ret = i2c_write_mpu6050(client, SMPLRT_DIV, 0x07);
    if (ret < 0) goto init_fail;

    /* 配置通用配置寄存器：低通滤波器的设置，截止频率是1K，带宽是5K */
    ret = i2c_write_mpu6050(client, CONFIG, 0x06);
    if (ret < 0) goto init_fail;

    /* 配置加速度计配置寄存器：配置加速度传感器工作在2G模式，不自检 */
    ret = i2c_write_mpu6050(client, ACCEL_CONFIG, 0x00);
    if (ret < 0) goto init_fail;

    /* 配置陀螺仪配置寄存器：陀螺仪自检及测量范围，典型值：0x18(不自检，2000deg/s) */
    ret = i2c_write_mpu6050(client, GYRO_CONFIG, 0x18);
    if (ret < 0) goto init_fail;

    return 0;

init_fail:
    /* 初始化错误 */
    printk(KERN_ERR "mpu6050 init error \n");
    return -1;
}

/* 字符设备操作函数集，open函数实现 */
static int mpu6050_open(struct inode *inode, struct file *filp)
{
    /* 检查设备是否完成初始化 */
    if (mpu6050_dev.client == NULL) {
        printk(KERN_ERR "mpu6050 not initialized \n");
        return -ENODEV;
    }
    
    /* 打印设备打开信息 */
    printk("mpu6050 open\n");

    return 0;
}

/* 字符设备操作函数集，.read函数实现 */
static ssize_t mpu6050_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
    int ret;

    /* 连续读取14字节 = 加速度6字节 + 温度2字节 + 陀螺仪6字节 */
    u8 read_buf[14] = {0};

    /* 保存数据，3轴加速度 + 温度 + 3轴陀螺仪，共7个short型数据 */
    short mpu6050_result[7] = {0};

    /*
     * 0x3B~0x48寄存器
     * 对应ACCEL_XOUT_H~GYRO_ZOUT_L
     * 依次为ACCEL_X、Y、Z、温度、GYRO_X、Y、Z的高8位和低8位共14个寄存器
     */

    /* 从 ACCEL_XOUT_H(0x3B) 开始连续读取14字节 (读0x3B~0x48寄存器)，一次I2C传输全部读取完 */
    ret = i2c_read_mpu6050(mpu6050_dev.client, ACCEL_XOUT_H, read_buf, sizeof(read_buf));
    if (ret < 0) {
        return -EIO;
    }

    /* 
     * 解析数据：提取加速度+温度+陀螺仪
     * 0~5  ：加速度 X/Y/Z
     * 6~7  ：温度
     * 8~13 ：陀螺仪 X/Y/Z
     * MPU6050数据为高字节在前，低字节在后，拼接为16位short
     */
    mpu6050_result[0] = (read_buf[0] << 8) | read_buf[1];   // ACCEL_X 加速度X
    mpu6050_result[1] = (read_buf[2] << 8) | read_buf[3];   // ACCEL_Y 加速度Y
    mpu6050_result[2] = (read_buf[4] << 8) | read_buf[5];   // ACCEL_Z 加速度Z

    mpu6050_result[3] = (read_buf[6] << 8) | read_buf[7];   // TEMP    温度原始值

    mpu6050_result[4] = (read_buf[8] << 8)  | read_buf[9];  // GYRO_X  陀螺仪X
    mpu6050_result[5] = (read_buf[10] << 8) | read_buf[11]; // GYRO_Y  陀螺仪Y
    mpu6050_result[6] = (read_buf[12] << 8) | read_buf[13]; // GYRO_Z  陀螺仪Z

    /* 限制拷贝长度，防止用户传入的cnt过小导致越界 */
    cnt = min(cnt, sizeof(mpu6050_result));

    /* 将读取得到的数据拷贝到用户空间 */
    ret = copy_to_user(buf, mpu6050_result, cnt);

    /* ret > 0 说明数据没有完整拷贝到用户空间 */
    if (ret > 0) return -EFAULT;
    
    return cnt;
}

/* 字符设备操作函数集，.release函数实现 */
static int mpu6050_release(struct inode *inode, struct file *filp)
{
    printk("mpu6050 release\r\n");

    return 0;
}

/* 字符设备操作函数集 */
static struct file_operations mpu6050_chr_dev_fops =
{
    .owner = THIS_MODULE,
    .open = mpu6050_open,
    .read = mpu6050_read,
    .release = mpu6050_release,
};

/* i2c总线设备函数集 */
static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    /* 定义返回值变量 */
    int ret = 0;
    /* 定义主设备号变量 */
    int major;
    /* 定义次设备号变量 */
    int minor;

    printk("mpu6050 driver probe\n");

    /* 分配设备号 */
    ret = alloc_chrdev_region(&mpu6050_dev.devno, 0, DEV_CNT, DEV_NAME);
    if (ret < 0) {
        /* 打印设备号分配失败信息 */
        printk("fail to alloc devno\n");
        /* 跳转到错误处理标签 */
        goto alloc_err;
    }
    /* 获取主设备号 */
    major = MAJOR(mpu6050_dev.devno);
    /* 获取次设备号 */
    minor = MINOR(mpu6050_dev.devno);
    /* 打印主设备号和次设备号 */
    printk("major=%d, minor=%d\n", major, minor);

    /* 初始化字符设备 */
    cdev_init(&mpu6050_dev.dev, &mpu6050_chr_dev_fops);
    mpu6050_dev.dev.owner = THIS_MODULE;

    /* 添加字符设备 */
    ret = cdev_add(&mpu6050_dev.dev, mpu6050_dev.devno, DEV_CNT);
    if (ret < 0) {
        /* 打印字符设备添加失败信息 */
        printk("fail to add cdev\n");
        /* 跳转到错误处理标签 */
        goto add_err;
    }

    /* 创建设备类 */
    mpu6050_dev.class = class_create(THIS_MODULE, DEV_NAME);
    if (IS_ERR(mpu6050_dev.class)) {
        /* 打印设备类创建失败信息 */
        printk("fail to create class\n");
        ret = PTR_ERR(mpu6050_dev.class);
        /* 跳转到错误处理标签 */
        goto class_err;
    }

    /* 创建设备节点 */
    mpu6050_dev.device = device_create(mpu6050_dev.class, NULL, mpu6050_dev.devno, NULL, DEV_NAME);
    if (IS_ERR(mpu6050_dev.device)) {
        /* 打印设备节点创建失败信息 */
        printk("fail to create device\n");
        ret = PTR_ERR(mpu6050_dev.device);
        /* 跳转到错误处理标签 */
        goto device_err;
    }

    /* 保存 I2C 设备信息，赋值到封装结构体的 client 成员 */
    mpu6050_dev.client = client;

    /* 向 mpu6050 发送配置数据，让mpu6050处于正常工作状态 */
    ret = mpu6050_init(mpu6050_dev.client);
    if (ret < 0) {
        pr_err("mpu6050 hardware init failed\n");
        goto init_err;
    }

    return 0;
    
init_err:
    /* 销毁设备节点 */
    device_destroy(mpu6050_dev.class, mpu6050_dev.devno);

device_err:
    /* 销毁设备类 */
    class_destroy(mpu6050_dev.class);

class_err:
    /* 删除字符设备 */
    cdev_del(&mpu6050_dev.dev);

add_err:
    /* 释放设备号 */
    unregister_chrdev_region(mpu6050_dev.devno, DEV_CNT);

alloc_err:
    return ret;
}

static int mpu6050_remove(struct i2c_client *client)
{
    /* 打印平台驱动移除信息 */
    printk("mpu6050 driver remove\n");

    /* 销毁设备节点 */
    device_destroy(mpu6050_dev.class, mpu6050_dev.devno);

    /* 删除字符设备 */
    cdev_del(&mpu6050_dev.dev);

    /* 释放设备号 */
    unregister_chrdev_region(mpu6050_dev.devno, DEV_CNT);

    /* 销毁设备类 */
    class_destroy(mpu6050_dev.class);

    /* 置空client指针，防止野指针 */
    mpu6050_dev.client = NULL;

    return 0;
}

/* 定义设备树匹配表 */
static const struct of_device_id mpu6050_of_match_table[] = {
    {.compatible = "fire,i2c_mpu6050"},
    {}
};

MODULE_DEVICE_TABLE(of, mpu6050_of_match_table);

/* 定义i2c总线设备结构体 */
struct i2c_driver mpu6050_driver = {
    .probe = mpu6050_probe,
    .remove = mpu6050_remove,
    .driver = {
        .name = "fire,i2c_mpu6050",
        .owner = THIS_MODULE,
        .of_match_table = mpu6050_of_match_table,
    },
};

/*
 * 驱动初始化函数
 */
static int __init mpu6050_driver_init(void)
{
    int ret;

    printk("mpu6050 driver init\n");

    /* 向Linux I2C子系统注册MPU6050驱动 */
    ret = i2c_add_driver(&mpu6050_driver);

    return ret;
}

/*
 * 驱动注销函数
 */
static void __exit mpu6050_driver_exit(void)
{
    printk("mpu6050 driver exit\n");

    /* 从Linux I2C核心子系统注销MPU6050驱动，释放驱动资源 */
    i2c_del_driver(&mpu6050_driver);
}

module_init(mpu6050_driver_init);
module_exit(mpu6050_driver_exit);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("i2c_mpu6050 module");
MODULE_LICENSE("GPL");