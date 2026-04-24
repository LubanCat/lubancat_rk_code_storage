#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/mod_devicetable.h>
#include <linux/regmap.h>
#include <linux/iio/iio.h>
#include <linux/of.h>
#include <linux/delay.h>

#define ADS1115_REG_CONVERSION    0x00              /* 定义ADS1115转换寄存器地址，存储ADC采样原始值 */
#define ADS1115_REG_CONFIG        0x01              /* 定义ADS1115配置寄存器地址，设置ADC工作模式/通道/增益/采样率 */

#define ADS1115_CFG_OS            BIT(15)           /* 配置寄存器bit15：单次转换启动位，写1启动ADC转换 */
#define ADS1115_CFG_MODE          BIT(8)            /* 配置寄存器bit8：工作模式位，1=单次转换模式，0=连续转换模式 */

/* 定义PGA增益对应表：寄存器配置值 -> 满量程电压，单位mV */
static const unsigned int ads1115_pga_mv_table[] = {
    6144,   /* 0: 寄存器值0 -> ±6.144V满量程 */
    4096,   /* 1: 寄存器值1 -> ±4.096V满量程 */
    2048,   /* 2: 寄存器值2 -> ±2.048V满量程 */
    1024,   /* 3: 寄存器值3 -> ±1.024V满量程 */
    512,    /* 4: 寄存器值4 -> ±0.512V满量程 */
    256,    /* 5: 寄存器值5 -> ±0.256V满量程 */
    256,    /* 6: 寄存器值6 -> ±0.256V满量程 */
    256,    /* 7: 寄存器值7 -> ±0.256V满量程 */
};
/* 定义默认PGA增益：0 -> ±6.144V满量程 */
#define ADS1115_DEFAULT_PGA 0

/* 定义采样率对应表：寄存器配置值 -> 采样速率，单位SPS */
static const unsigned int ads1115_dr_sps_table[] = {
    8,      /* 0: 寄存器值0 -> 8采样点/秒 */
    16,     /* 1: 寄存器值1 -> 16采样点/秒 */
    32,     /* 2: 寄存器值2 -> 32采样点/秒 */
    64,     /* 3: 寄存器值3 -> 64采样点/秒 */
    128,    /* 4: 寄存器值4 -> 128采样点/秒 */
    250,    /* 5: 寄存器值5 -> 250采样点/秒 */
    475,    /* 6: 寄存器值6 -> 475采样点/秒 */
    860,    /* 7: 寄存器值7 -> 860采样点/秒 */
};
/* 定义默认采样率：4 -> 128采样点/秒 */
#define ADS1115_DEFAULT_DR  4

/* 定义ADS1115驱动私有数据结构体 */
struct ads1115_data {
    struct i2c_client *client;  /* I2C客户端句柄，关联I2C从设备 */
    struct regmap *regmap;      /* Regmap句柄，用于寄存器统一读写 */
    unsigned int pga;           /* 保存当前PGA增益配置值 */
    unsigned int dr;            /* 保存当前采样率配置值 */
};

/* 配置Regmap参数 */
static const struct regmap_config ads1115_regmap_config = {
    .reg_bits = 8,                          /* 寄存器地址位宽：8位 */
    .val_bits = 16,                         /* 寄存器值位宽：16位 */
    .val_format_endian = REGMAP_ENDIAN_BIG, /* 寄存器值为大端字节序 */
    .max_register = ADS1115_REG_CONFIG,     /* 最大可访问寄存器地址 */
    .cache_type = REGCACHE_NONE,            /* 关闭寄存器缓存，保证实时性 */
};

/* 定义IIO通道宏 */
#define ADS1115_CHANNEL(_chan) {    \
    .type = IIO_VOLTAGE,        /* 通道类型：电压采集 */    \
    .indexed = 1,               /* 启用通道索引标识 */      \
    .channel = _chan,           /* 通道编号：0/1/2/3 */     \
    .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),    /* 生成sysfs节点：原始值(raw) + 缩放系数(scale) */  \
    .scan_type = {  \
        .sign = 's',            /* 数据类型：有符号数 */    \
        .realbits = 16,         /* ADC有效分辨率：16位 */   \
        .storagebits = 16,      /* 数据存储位宽：16位 */    \
        .endianness = IIO_CPU,  /* 数据字节序：CPU本地序 */ \
    },  \
}

/* 定义ADS1115的4路电压采集通道 */
static const struct iio_chan_spec ads1115_channels[] = {
    ADS1115_CHANNEL(0), /* 通道0：AIN0 */
    ADS1115_CHANNEL(1), /* 通道1：AIN1 */
    ADS1115_CHANNEL(2), /* 通道2：AIN2 */
    ADS1115_CHANNEL(3), /* 通道3：AIN3 */
};

/* IIO子系统读取回调函数，处理RAW原始值和SCALE缩放系数读取 */
static int ads1115_read_raw(struct iio_dev *indio_dev,
                struct iio_chan_spec const *chan,
                int *val, int *val2, long mask)
{
    /* 获取驱动私有数据结构体指针 */
    struct ads1115_data *data = iio_priv(indio_dev);
    /* 定义寄存器值临时变量 */
    unsigned int reg_val;
    /* 定义16位有符号ADC原始值变量 */
    s16 raw_adc;
    /* 定义配置寄存器值变量 */
    u16 config;
    /* 定义函数返回值变量 */
    int ret;

    /* 判断读取类型：原始值/缩放系数 */
    switch (mask) {
    case IIO_CHAN_INFO_RAW: /* 读取ADC原始采样值 */
        /* 拼接配置寄存器值：启动转换+通道选择+PGA+单次模式+采样率 */
        config = ADS1115_CFG_OS |           /* 启动单次转换 */
            ((chan->channel + 4) << 12) |   /* 设置通道chan->channel，如ADS1115_CHANNEL(0) + 4 得到bit值为100，左移12位到bit 14:12位，即可选择AINP=AIN0和AINN=GND */
            (data->pga << 9) |              /* 设置PGA增益data->pga */
            ADS1115_CFG_MODE |              /* 设置单次模式 */
            (data->dr << 5);                /* 设置采样率data->dr */

        /* 通过Regmap写入配置寄存器，启动ADC转换 */
        ret = regmap_write(data->regmap, ADS1115_REG_CONFIG, config);
        if (ret)
            return ret;

        /* 根据采样率计算转换延时，等待ADC采样完成 */
        msleep(DIV_ROUND_UP(1000, ads1115_dr_sps_table[data->dr]));

        /* 通过Regmap读取转换结果寄存器值 */
        ret = regmap_read(data->regmap, ADS1115_REG_CONVERSION, &reg_val);
        if (ret)
            return ret;

        /* 将16位无符号寄存器值强转为有符号数 */
        raw_adc = (s16)reg_val;

        /* 将转换后的原始值存入输出参数 */
        *val = raw_adc;

        /* 返回标识：读取的数据为纯整数类型 */
        return IIO_VAL_INT;

    case IIO_CHAN_INFO_SCALE:   /* 读取电压缩放系数 */
        /* 缩放系数=满量程mV值，配合2^15计算实际电压 */
        *val = ads1115_pga_mv_table[data->pga];

        /* 位移值：15位，2^15=32768（16位有符号数最大值） */
        *val2 = 15;

        /* 返回标识：2的指数分数类型 val/(2^val2) */
        return IIO_VAL_FRACTIONAL_LOG2;

    default:    /* 未知读取类型，返回参数错误 */
        return -EINVAL;
    }
}

/* 定义IIO操作函数集，绑定read_raw回调给内核IIO子系统 */
static const struct iio_info ads1115_iio_info = {
    .read_raw = ads1115_read_raw,
};

/* 从设备树中读取PGA增益和采样率配置参数 */
static void ads1115_of_get_config(struct ads1115_data *data)
{
    /* 获取设备指针，关联I2C客户端设备 */
    struct device *dev = &data->client->dev;
    /* 定义设备树属性读取临时变量 */
    unsigned int val;

    /* 读取设备树gain属性，获取PGA增益配置 */
    if (!of_property_read_u32(dev->of_node, "gain", &val)) {
        /* 校验增益值是否在合法范围内 */
        if (val < ARRAY_SIZE(ads1115_pga_mv_table))
            data->pga = val;    /* 合法则使用设备树配置 */
        else
            data->pga = ADS1115_DEFAULT_PGA; /* 非法则使用默认增益 */
    } else {
        data->pga = ADS1115_DEFAULT_PGA;     /* 无配置则使用默认增益 */
    }

    /* 读取设备树data-rate属性，获取采样率配置 */
    if (!of_property_read_u32(dev->of_node, "data-rate", &val)) {
        /* 校验采样率值是否在合法范围内 */
        if (val < ARRAY_SIZE(ads1115_dr_sps_table))
            data->dr = val;     /* 合法则使用设备树配置 */
        else
            data->dr = ADS1115_DEFAULT_DR;  /* 非法则使用默认采样率 */
    } else {
        data->dr = ADS1115_DEFAULT_DR;      /* 无配置则使用默认采样率 */
    }

    /* 内核日志打印当前PGA和采样率配置 */
    dev_info(dev, "ADS1115: PGA=%dmV, DataRate=%uSPS\n",
        ads1115_pga_mv_table[data->pga],
        ads1115_dr_sps_table[data->dr]);
}

/* I2C设备探测函数，设备匹配成功后自动调用 */
static int ads1115_probe(struct i2c_client *client,
            const struct i2c_device_id *id)
{
    /* 定义IIO设备结构体指针 */
    struct iio_dev *indio_dev;
    /* 定义驱动私有数据结构体指针 */
    struct ads1115_data *data;
    /* 定义函数返回值变量 */
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
    data->regmap = devm_regmap_init_i2c(client, &ads1115_regmap_config);
    if (IS_ERR(data->regmap)) {
        dev_err(&client->dev, "regmap init failed\n");
        return PTR_ERR(data->regmap);
    }

    /* 从设备树加载ADC配置参数 */
    ads1115_of_get_config(data);

    /* 设置IIO设备名称 */
    indio_dev->name = "ads1115";
    /* 绑定IIO操作函数集 */
    indio_dev->info = &ads1115_iio_info;
    /* 绑定IIO通道配置数组 */
    indio_dev->channels = ads1115_channels;
    /* 设置通道数量为4路 */
    indio_dev->num_channels = ARRAY_SIZE(ads1115_channels);
    /* 设置IIO工作模式：直接访问模式 */
    indio_dev->modes = INDIO_DIRECT_MODE;

    /* 注册IIO设备到内核，自动生成sysfs节点 */
    ret = devm_iio_device_register(&client->dev, indio_dev);
    /* 注册失败则打印错误并返回 */
    if (ret) {
        dev_err(&client->dev, "iio device register failed\n");
        return ret;
    }

    dev_info(&client->dev, "ADS1115 IIO driver probe successful\n");
    return 0;
}

/* I2C设备移除函数 */
static int ads1115_remove(struct i2c_client *client)
{
    dev_info(&client->dev, "ADS1115 IIO driver removed\n");
    return 0;
}

/* 定义设备树匹配表 */
static const struct of_device_id ads1115_of_match[] = {
    { .compatible = "fire,ads1115" },
    { }
};

/* 声明设备树匹配表，供内核识别 */
MODULE_DEVICE_TABLE(of, ads1115_of_match);

/* 定义i2c总线设备结构体 */
static struct i2c_driver ads1115_driver = {
    .probe = ads1115_probe,
    .remove = ads1115_remove,
    .driver = {
        .name = "adc-ads1115",
        .of_match_table = ads1115_of_match,
    },
};

/* 注册/注销I2C驱动，简化模块入口/出口函数 */
module_i2c_driver(ads1115_driver);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("adc_ads1115 module");
MODULE_LICENSE("GPL");