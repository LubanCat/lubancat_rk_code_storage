#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/mod_devicetable.h>
#include <linux/regmap.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

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

/* 驱动私有数据结构体 */
struct mpu6050_data {
    struct i2c_client *client;   /* 指向I2C从设备客户端，存储设备信息 */
    struct regmap *regmap;       /* regmap映射句柄，简化寄存器读写 */
};

/* regmap配置结构体 */
static const struct regmap_config mpu6050_regmap_config = {
    .reg_bits = 8,               /* 寄存器地址位宽：8位 */
    .val_bits = 8,               /* 寄存器值位宽：8位 */
    .can_multi_write = true,     /* 支持多字节批量写入 */
};

/* IIO通道索引枚举 */
enum mpu6050_channel_idx {
    CHAN_ACCEL_X,                /* 加速度X轴通道索引 */
    CHAN_ACCEL_Y,                /* 加速度Y轴通道索引 */
    CHAN_ACCEL_Z,                /* 加速度Z轴通道索引 */
    CHAN_TEMP,                   /* 温度通道索引 */
    CHAN_ANGL_VEL_X,             /* 陀螺仪X轴通道索引 */
    CHAN_ANGL_VEL_Y,             /* 陀螺仪Y轴通道索引 */
    CHAN_ANGL_VEL_Z,             /* 陀螺仪Z轴通道索引 */
};

/* IIO通道描述数组：定义每个传感器通道的类型、属性、sysfs节点 */
static const struct iio_chan_spec mpu6050_channels[] = {
    /* 加速度计X轴通道配置 */
    {
        .type = IIO_ACCEL,                    /* 通道类型：加速度计 */
        .modified = 1,                        /* 启用轴方向修饰符 */
        .channel2 = IIO_MOD_X,                /* 轴方向：X轴 */
        .address = CHAN_ACCEL_X,              /* 通道索引：对应枚举值 */
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),    /* 生成sysfs节点：原始值(raw) + 缩放系数(scale) */
    },
    /* 加速度计Y轴通道配置 */
    {
        .type = IIO_ACCEL,
        .modified = 1,
        .channel2 = IIO_MOD_Y,
        .address = CHAN_ACCEL_Y,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),
    },
    /* 加速度计Z轴通道配置 */
    {
        .type = IIO_ACCEL,
        .modified = 1,
        .channel2 = IIO_MOD_Z,
        .address = CHAN_ACCEL_Z,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),
    },
    /* 温度传感器通道配置 */
    {
        .type = IIO_TEMP,                     /* 通道类型：温度 */
        .address = CHAN_TEMP,                 /* 通道索引 */
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),
    },
    /* 陀螺仪X轴通道配置 */
    {
        .type = IIO_ANGL_VEL,                 /* 通道类型：陀螺仪 */
        .modified = 1,
        .channel2 = IIO_MOD_X,
        .address = CHAN_ANGL_VEL_X,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),
    },
    /* 陀螺仪Y轴通道配置 */
    {
        .type = IIO_ANGL_VEL,
        .modified = 1,
        .channel2 = IIO_MOD_Y,
        .address = CHAN_ANGL_VEL_Y,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),
    },
    /* 陀螺仪Z轴通道配置 */
    {
        .type = IIO_ANGL_VEL,
        .modified = 1,
        .channel2 = IIO_MOD_Z,
        .address = CHAN_ANGL_VEL_Z,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),
    },
};

/*
 * 函数：IIO 原始数据读取回调函数
 * indio_dev: IIO设备结构体
 * chan: 当前读取的通道描述符
 * val: 存储读取的整数值
 * val2: 存储小数/缩放值
 * mask: 读取类型（RAW/scale）
 */
static int mpu6050_read_raw(struct iio_dev *indio_dev,
                           struct iio_chan_spec const *chan,
                           int *val, int *val2, long mask)
{
    /* 获取驱动私有数据结构体 */
    struct mpu6050_data *data = iio_priv(indio_dev);
    /* 存储16位原始传感器数据 */
    u16 raw_data;
    /* 存储要读取的寄存器地址 */
    u8 reg;

    /* 判断读取类型：原始数据/缩放系数 */
    switch (mask) {
        /* 读取传感器原始数据（RAW） */
        case IIO_CHAN_INFO_RAW:
            /* 根据通道索引选择对应寄存器地址 */
            switch (chan->address) {
                case CHAN_ACCEL_X: reg = ACCEL_XOUT_H; break;       /* 加速度计X轴数据高8位地址 */
                case CHAN_ACCEL_Y: reg = ACCEL_YOUT_H; break;
                case CHAN_ACCEL_Z: reg = ACCEL_ZOUT_H; break;
                case CHAN_TEMP:    reg = TEMP_OUT_H; break;         /* 温度传感器数据高8位地址 */
                case CHAN_ANGL_VEL_X:  reg = GYRO_XOUT_H; break;    /* 陀螺仪X轴数据高8位地址 */
                case CHAN_ANGL_VEL_Y:  reg = GYRO_YOUT_H; break;
                case CHAN_ANGL_VEL_Z:  reg = GYRO_ZOUT_H; break;

                /* 无效通道，返回参数错误 */
                default: return -EINVAL;
            }

            /* 连续读取2字节寄存器数据，高8位+低8位 */
            regmap_bulk_read(data->regmap, reg, &raw_data, 2);

            /* 
             * 1、需使用be16_to_cpu将把传感器的大端16位数转换成CPU能识别的小端数
             * 原因是：
             * MPU6050的16位数据是大端模式：先输出高8位寄存器，后输出低8位寄存器
             * CPU是ARM架构小端模式，低字节存放在低地址，直接读取会把数据解析成0x3412，需转为0x4321
             * 2、需使用(s16)解决有符号数（补码）问题
             * 原因是：
             * 加速度、陀螺仪、温度都是16位有符号补码，可以是负数
             * 寄存器直接输出二进制补码
             * 读取后的默认类型raw_data是u16，无符号16位整数
             * 如果不强制转换，负数补码会被解析成极大的正数
             * 使用(s16)强制转换，告诉编译器这是16位有符号数，正确解析正负值
             */

            *val = (s16)be16_to_cpu(raw_data);

            /* 返回值类型：整数值，没有小数 */
            return IIO_VAL_INT;

        /* 读取传感器缩放系数（scale） */
        case IIO_CHAN_INFO_SCALE:
            /* 根据通道类型返回对应缩放系数 */
            switch (chan->type) {
                case IIO_ACCEL:
                    *val = 0;    *val2 = 61035;    /* 加速度灵敏度 = 量程 / ADC精度(16位有符号数最大值是32768) = 2g / 32768 = 0.000061035 g/LSB */
                    break;
                case IIO_ANGL_VEL:
                    *val = 0;    *val2 = 61035156; /* 陀螺仪灵敏度 = 量程 / ADC精度(16位有符号数最大值是32768) = 2000°/s / 32768 = 0.061035156 °/s/LSB */
                    break;
                case IIO_TEMP:
                    *val = 0;    *val2 = 2941176;  /* 温度灵敏度 = 340 LSB/℃ = 1 / 340 ℃/LSB = 0.002941176 ℃/LSB */
                    break;

                /* 无效类型，返回参数错误 */
                default: return -EINVAL;
            }

            /* 返回值类型：整数+纳位小数，即val + val2 / 1000000000 */
            return IIO_VAL_INT_PLUS_NANO;

        default: return -EINVAL;    /* 无效命令，返回参数错误 */
    }
}

/* IIO操作函数集：绑定read_raw回调，供IIO子系统调用 */
static const struct iio_info mpu6050_iio_info = {
    .read_raw = mpu6050_read_raw,
};

/* MPU6050 硬件初始化函数：配置芯片工作模式、量程、唤醒芯片 */
static int mpu6050_init(struct mpu6050_data *data)
{
    /* 函数返回值 */
    int ret;
    /* 存储寄存器读取值 */
    unsigned int val;

    /* 读取WHO_AM_I寄存器，校验芯片是否正常 */
    ret = regmap_read(data->regmap, WHO_AM_I, &val);
    if (ret || val != 0x68) {
        dev_err(&data->client->dev, "WHO_AM_I error: 0x%x\n", val);
        return -ENODEV;
    }

    /* 配置电源管理寄存器：唤醒MPU6050，使用内部8Mhz时钟源 */
    ret = regmap_write(data->regmap, PWR_MGMT_1, 0x00);
    if (ret < 0) goto init_fail;

    /* 配置采样率分频寄存器：陀螺仪采样率，1KHz */
    ret = regmap_write(data->regmap, SMPLRT_DIV, 0x07);
    if (ret < 0) goto init_fail;

    /* 配置通用配置寄存器：低通滤波器的设置，截止频率是1K，带宽是5K */
    ret = regmap_write(data->regmap, CONFIG, 0x06);
    if (ret < 0) goto init_fail;

    /* 配置加速度计配置寄存器：配置加速度传感器工作在2G模式，不自检 */
    ret = regmap_write(data->regmap, ACCEL_CONFIG, 0x00);
    if (ret < 0) goto init_fail;

    /* 配置陀螺仪配置寄存器：陀螺仪自检及测量范围，典型值：0x18(不自检，2000deg/s) */
    ret = regmap_write(data->regmap, GYRO_CONFIG, 0x18);
    if (ret < 0) goto init_fail;

    dev_info(&data->client->dev, "MPU6050 init success\n");
    return 0;

init_fail:
    /* 初始化错误 */
    printk(KERN_ERR "mpu6050 init error \n");
    return ret;
}

/*
 * I2C探测函数：设备匹配成功后自动调用
 * client: I2C从设备客户端
 * id: I2C设备ID匹配表
 */
static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    /* IIO设备结构体指针 */
    struct iio_dev *indio_dev;
    /* 驱动私有数据指针 */
    struct mpu6050_data *data;
    /* 函数返回值 */
    int ret;

    /* 动态分配IIO设备内存 + 私有数据内存 */
    indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
    if (!indio_dev)
        return -ENOMEM;

    /* 获取私有数据结构体地址 */
    data = iio_priv(indio_dev);

    /* 绑定I2C客户端 */
    data->client = client;

    /* 初始化I2C regmap映射 */
    data->regmap = devm_regmap_init_i2c(client, &mpu6050_regmap_config);
    if (IS_ERR(data->regmap)) {
        dev_err(&client->dev, "regmap init failed\n");
        return PTR_ERR(data->regmap);
    }

    /* 调用MPU6050硬件初始化函数 */
    ret = mpu6050_init(data);
    if (ret)
        return ret;

    /* 配置IIO设备参数 */
    indio_dev->info = &mpu6050_iio_info;       /* 绑定IIO操作函数集 */
    indio_dev->channels = mpu6050_channels;    /* 绑定通道描述数组 */
    indio_dev->num_channels = ARRAY_SIZE(mpu6050_channels); /* 通道数量：自动计算数组大小 */
    indio_dev->name = "mpu6050";                /* IIO设备名称 */
    indio_dev->modes = INDIO_DIRECT_MODE;       /* 直接访问模式，无缓冲 */

    /* 注册IIO设备到内核，自动生成sysfs节点 */
    ret = devm_iio_device_register(&client->dev, indio_dev);
    if (ret) {
        dev_err(&client->dev, "IIO register failed\n");
        return ret;
    }

    dev_info(&client->dev, "MPU6050 IIO driver probe success\n");
    return 0;
}

/* I2C设备移除函数 */
static int mpu6050_remove(struct i2c_client *client)
{
    dev_info(&client->dev, "MPU6050 IIO driver removed\n");
    return 0;
}

/* 定义设备树匹配表 */
static const struct of_device_id mpu6050_of_match_table[] = {
    { .compatible = "fire,iio_mpu6050" },
    { }
};

/* 声明设备树匹配表，供内核识别 */
MODULE_DEVICE_TABLE(of, mpu6050_of_match_table);

/* 定义i2c总线设备结构体 */
static struct i2c_driver mpu6050_driver = {
    .probe = mpu6050_probe,
    .remove = mpu6050_remove,
    .driver = {
        .name = "iio-mpu6050",
        .of_match_table = mpu6050_of_match_table,
    },
};

/* 注册/注销I2C驱动，简化模块入口/出口函数 */
module_i2c_driver(mpu6050_driver);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("iio_mpu6050 module");
MODULE_LICENSE("GPL");