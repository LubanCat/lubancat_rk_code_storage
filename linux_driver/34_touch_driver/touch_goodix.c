#include <linux/kernel.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/input/touchscreen.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/acpi.h>
#include <linux/of.h>
#include <asm/unaligned.h>
#include <linux/regmap.h>

/* 触摸硬件参数宏定义 */
#define GOODIX_MAX_HEIGHT           4096     /* 触摸屏最大Y轴分辨率 */
#define GOODIX_MAX_WIDTH            4096     /* 触摸屏最大X轴分辨率 */
#define GOODIX_CONTACT_SIZE         8        /* 单个触摸点数据长度 */
#define GOODIX_MAX_CONTACTS         10       /* 驱动支持的最大触摸点数 */

/* 芯片配置数据长度宏定义 */
#define GOODIX_CONFIG_911_LENGTH    186      /* GT911/GT928的配置信息寄存器长度 */

/* 芯片寄存器地址宏定义 */
#define GOODIX_REG_COMMAND          0x8040   /* 命令控制寄存器地址 */
#define GOODIX_READ_COOR_ADDR       0x814E   /* 状态位寄存器地址 */
#define GOODIX_GT9X_REG_CONFIG_DATA 0x8047   /* GT911/GT928配置信息寄存器开始地址 */
#define GOODIX_REG_ID               0x8140   /* GT911/GT928坐标信息寄存器开始地址 */

/* 触摸数据状态位宏定义 */
#define GOODIX_BUFFER_STATUS_READY  BIT(7)   /* 数据缓冲区就绪状态位 */
#define GOODIX_BUFFER_STATUS_TIMEOUT 20      /* 数据就绪等待超时时间 */

/* 配置数据偏移宏定义 */
#define RESOLUTION_LOC      1        /* 分辨率参数在配置区的偏移位置 */
#define MAX_CONTACTS_LOC    5        /* 最大触摸点数在配置区的偏移位置 */
#define TRIGGER_LOC         6        /* 中断触发类型在配置区的偏移位置 */

/* 中断触发类型数组 */
static const unsigned long goodix_irq_flags[] = {
    IRQ_TYPE_EDGE_RISING,           /* 索引0：上升沿触发 */
    IRQ_TYPE_EDGE_FALLING,          /* 索引1：下降沿触发 */
    IRQ_TYPE_LEVEL_LOW,             /* 索引2：低电平触发 */
    IRQ_TYPE_LEVEL_HIGH             /* 索引3：高电平触发 */
};

#define GOODIX_INT_TRIGGER  1       /* 默认中断触发类型索引，索引1下降沿触发 */

/* 芯片配置数据结构体 */
struct goodix_chip_data {
    u16 config_addr;    /* 配置信息寄存器的起始地址 */
    int config_len;     /* 配置信息寄存器的总长度 */
};

/* 触摸驱动核心数据结构体 */
struct goodix_ts_data {
    struct i2c_client *client;           /* I2C客户端设备结构体 */
    struct regmap *regmap;               /* Regmap操作句柄 */
    struct input_dev *input_dev;         /* 输入设备结构体 */
    const struct goodix_chip_data *chip; /* 指向当前芯片的配置参数结构体 */
    struct touchscreen_properties prop;  /* 触摸屏物理属性结构体 */
    unsigned int max_touch_num;          /* 设备支持的最大触摸点数量 */
    unsigned int int_trigger_type;       /* 中断触发类型 */
    struct gpio_desc *gpiod_int;         /* 中断GPIO描述符 */
    struct gpio_desc *gpiod_rst;         /* 复位GPIO描述符 */
    u16 id;                              /* 芯片型号ID */
    u16 version;                         /* 芯片固件版本号 */
    unsigned long irq_flags;             /* 中断注册标志位 */
};

/* Regmap配置结构体 */
static const struct regmap_config goodix_regmap_config = {
    .reg_bits = 16,        /* 寄存器地址位宽：16位 */
    .val_bits = 8,         /* 数据位宽：8位 */
    .write_flag_mask = 0,  /* 写操作无额外标志位 */
    .read_flag_mask = 0,   /* 读操作无额外标志位 */
    .fast_io = true,       /* 开启快速I/O模式，满足触摸高实时性需求 */
};

/*
* GT911/GT928芯片专用配置参数
* 两款芯片寄存器地址、配置长度完全一致，共用该结构体
*/
static const struct goodix_chip_data gt911_chip_data = {
    .config_addr     = GOODIX_GT9X_REG_CONFIG_DATA,  /* 配置信息寄存器起始地址 */
    .config_len      = GOODIX_CONFIG_911_LENGTH,     /* 配置信息寄存器数据长度 */
};

/* 根据芯片ID获取对应配置参数 */
static const struct goodix_chip_data *goodix_get_chip_data(u16 id)
{
    switch (id) {
    case 911:    /* 匹配GT911芯片ID */
    case 928:    /* 匹配GT928芯片ID */
        return &gt911_chip_data;  /* 返回共用配置参数 */
    default:     /* 不支持的芯片ID */
        return NULL;              /* 返回空指针标识不支持 */
    }
}

/*
* 读取触摸输入报告数据
* @ts: 驱动核心数据结构体指针
* @data: 存储读取到的触摸数据的缓冲区指针
* 返回值: 有效触摸点数量，负数表示读取失败
*/
static int goodix_ts_read_input_report(struct goodix_ts_data *ts, u8 *data)
{
    unsigned long max_timeout;  /* 数据等待超时的jiffies值 */
    int touch_num;              /* 有效触摸点数量 */
    int error;                  /* 函数执行错误码 */

    /* 计算超时时间：当前jiffies + 20ms转换的jiffies值 */
    max_timeout = jiffies + msecs_to_jiffies(GOODIX_BUFFER_STATUS_TIMEOUT);
    do {
        /* 通过regmap批量读取触摸坐标数据，状态位寄存器+第1个触摸点数据，0x814E~0x8156 */
        error = regmap_bulk_read(ts->regmap, GOODIX_READ_COOR_ADDR,
                                data, GOODIX_CONTACT_SIZE + 1);
        if (error) {
            dev_err(&ts->client->dev, "Regmap read error: %d\n", error);
            return error;
        }
        /* 判断数据缓冲区是否就绪，0x814E第7位为1表示就绪 */
        if (data[0] & GOODIX_BUFFER_STATUS_READY) { 
            touch_num = data[0] & 0x0f;         /* 提取低4位获取触摸点数量 */
            if (touch_num > ts->max_touch_num)  /* 触摸点数量超阈值则协议错误 */
                return -EPROTO;
            /* 触摸点数量大于1时，读取剩余触摸点数据 */
            if (touch_num > 1) {
                data += 1 + GOODIX_CONTACT_SIZE;  /* 缓冲区指针偏移，跳过已经存入的状态位+第1个触摸点数据 */
                error = regmap_bulk_read(ts->regmap,
                                        GOODIX_READ_COOR_ADDR + 1 + GOODIX_CONTACT_SIZE, /* 地址偏移状态位寄存器+第1个触摸点数据，从第二个触摸点开始读 */
                                        data, GOODIX_CONTACT_SIZE * (touch_num - 1));    /* 读取长度为一个触摸点长度 * 剩余的触摸点数 */
                if (error)
                    return error;
            }
            return touch_num;  /* 返回有效触摸点数量 */
        }
        usleep_range(1000, 2000);  /* 未就绪则休眠1-2ms后重试 */
    } while (time_before(jiffies, max_timeout));  /* 未超时则继续轮询 */

    /* 超时未读到数据，返回0个触摸点 */
    return 0;
}

/*
* 上报单个触摸点事件到输入子系统
* @ts: 驱动核心数据结构体指针
* @coor_data: 单个触摸点的原始坐标数据指针
*/
static void goodix_ts_report_touch(struct goodix_ts_data *ts, u8 *coor_data)
{
    /* 提取触摸点ID（低4位） */
    int id = coor_data[0] & 0x0F;
    /* 小端模式读取16位X轴坐标 */
    int input_x = get_unaligned_le16(&coor_data[1]);
    /* 小端模式读取16位Y轴坐标 */
    int input_y = get_unaligned_le16(&coor_data[3]);
    /* 小端模式读取16位触摸大小 */
    int input_w = get_unaligned_le16(&coor_data[5]);

    /* 绑定当前触摸点到对应MT槽位 */
    input_mt_slot(ts->input_dev, id);
    /* 上报槽位状态：当前为手指触摸，状态有效 */
    input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);
    /* 按触摸屏属性上报坐标，支持方向翻转 */
    touchscreen_report_pos(ts->input_dev, &ts->prop,
                input_x, input_y, true);
    /* 上报触摸主尺寸 */
    input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, input_w);
    /* 上报触摸宽度 */
    input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, input_w);
}

/*
* 处理所有触摸事件：读取数据并批量上报
* @ts: 驱动核心数据结构体指针
* 中断触发时调用，完成触摸数据读取与事件上报
*/
static void goodix_process_events(struct goodix_ts_data *ts)
{
    /* 定义触摸数据缓冲区：状态位寄存器+最大触摸点*单触摸点长度 */
    u8 point_data[1 + GOODIX_CONTACT_SIZE * GOODIX_MAX_CONTACTS];
    int touch_num;  /* 有效触摸点数量 */
    int i;          /* 循环遍历变量 */

    /* 读取所有触摸点数据 */
    touch_num = goodix_ts_read_input_report(ts, point_data);
    if (touch_num < 0)
        return;

    /* 循环上报所有有效触摸点事件 */
    for (i = 0; i < touch_num; i++) {
        goodix_ts_report_touch(ts, &point_data[1 + GOODIX_CONTACT_SIZE * i]);
    }
    /* 同步MT帧，标记一帧触摸数据上报完成 */
    input_mt_sync_frame(ts->input_dev);
    /* 发送同步事件，通知系统处理本次触摸数据 */
    input_sync(ts->input_dev);
}

/*
* 触摸中断处理函数
* @irq: 触发的中断号
* @dev_id: 设备私有数据指针
* 返回值: IRQ_HANDLED表示中断已处理完成
*/
static irqreturn_t goodix_ts_irq_handler(int irq, void *dev_id)
{
    /* 转换私有数据为驱动核心结构体 */
    struct goodix_ts_data *ts = dev_id;

    /* 调用事件处理函数，读取并上报触摸数据 */
    goodix_process_events(ts);
    /* 向0x814E状态位寄存器写入0，清除缓冲区状态标志和触摸点数量等 */
    if (regmap_write(ts->regmap, GOODIX_READ_COOR_ADDR, 0) < 0) {
        dev_err(&ts->client->dev, "Regmap write end_cmd error\n");
    }
    return IRQ_HANDLED;  /* 告知内核中断已处理 */
}

/*
* 释放触摸中断资源
* @ts: 驱动核心数据结构体指针
*/
static void goodix_free_irq(struct goodix_ts_data *ts)
{
    /* 释放设备树管理的中断资源 */
    devm_free_irq(&ts->client->dev, ts->client->irq, ts);
}

/*
* 申请触摸中断资源
* @ts: 驱动核心数据结构体指针
* 返回值: 0成功，负数为错误码
*/
static int goodix_request_irq(struct goodix_ts_data *ts)
{
    /* 申请线程化中断：无顶半部分，底半部分为中断处理函数 */
    return devm_request_threaded_irq(&ts->client->dev, ts->client->irq,
                                    NULL, goodix_ts_irq_handler,
                                    ts->irq_flags, ts->client->name, ts);
}

/*
* 中断GPIO初始化
* @ts: 驱动核心数据结构体指针
* 返回值: 0成功，负数为错误码
*/
static int goodix_int_sync(struct goodix_ts_data *ts)
{
    /* 函数执行错误码 */
    int error;

    /* 设置中断GPIO为输出模式，输出低电平 */
    error = gpiod_direction_output(ts->gpiod_int, 0);
    if (error)
        return error;
    msleep(50);
    /* 恢复中断GPIO为输入模式，等待芯片触发中断 */
    error = gpiod_direction_input(ts->gpiod_int);
    if (error)
        return error;
    return 0;
}

/*
* 芯片硬件复位
* @ts: 驱动核心数据结构体指针
* 返回值: 0成功，负数为错误码
*/
static int goodix_reset(struct goodix_ts_data *ts)
{   
    /* 函数执行错误码 */
    int error;

    /* 开始I2C从地址选择：复位GPIO置低 */
    error = gpiod_direction_output(ts->gpiod_rst, 0);
    if (error)
        return error;
    msleep(20); /* 延时20ms，满足芯片时序要求T2（>10ms） */

    /* 设置INT电平选择I2C地址: HIGH=0x14, LOW=0x5d */
    error = gpiod_direction_output(ts->gpiod_int, ts->client->addr == 0x14);
    if (error)
        return error;
    usleep_range(100, 2000); /* 延时100us-2ms，满足芯片时序要求T3（>100us） */

    /* 释放复位：复位GPIO置高 */
    error = gpiod_direction_output(ts->gpiod_rst, 1);
    if (error)
        return error;
    usleep_range(6000, 10000); /* 延时6-10ms，满足芯片时序要求T4（>5ms） */

    /* 复位GPIO恢复输入模式，结束I2C地址选择 */
    error = gpiod_direction_input(ts->gpiod_rst);
    if (error)
        return error;

    /* 执行中断GPIO初始化 */
    error = goodix_int_sync(ts);
    if (error)
        return error;

    return 0;
}

/*
* 从设备树获取GPIO配置
* @ts: 驱动核心数据结构体指针
* 返回值: 0成功，负数为错误码
*/
static int goodix_get_gpio_config(struct goodix_ts_data *ts)
{
    int error;               /* 函数执行错误码 */
    struct device *dev;      /* 设备结构体指针 */
    struct gpio_desc *gpiod; /* GPIO描述符临时指针 */

    if (!ts->client)  /* I2C客户端为空则参数错误 */
        return -EINVAL;
    dev = &ts->client->dev; /* 获取设备结构体 */

    /* 获取中断GPIO */
    gpiod = devm_gpiod_get(dev, "irq", GPIOD_IN);
    if (IS_ERR(gpiod)) {
        error = PTR_ERR(gpiod);
        if (error != -EPROBE_DEFER) {
            dev_err(dev, "Failed to get %s GPIO: %d\n",
                    "irq", error);
        }
        return error;
    }
    ts->gpiod_int = gpiod;  /* 保存中断GPIO描述符 */

    /* 获取复位GPIO */
    gpiod = devm_gpiod_get(dev, "reset", GPIOD_IN);
    if (IS_ERR(gpiod)) {
        error = PTR_ERR(gpiod);
        if (error != -EPROBE_DEFER) {
            dev_err(dev, "Failed to get %s GPIO: %d\n",
                    "reset", error);
        }
        return error;
    }
    ts->gpiod_rst = gpiod;  /* 保存复位GPIO描述符 */

    return 0;
}

/*
* 读取芯片内部配置参数
* @ts: 驱动核心数据结构体指针
* 读取分辨率、最大触摸点、中断触发类型等硬件配置
*/
static void goodix_read_config(struct goodix_ts_data *ts)
{
    u8 config[GOODIX_CONFIG_911_LENGTH]; /* 配置数据缓冲区 */
    int x_max, y_max;  /* X/Y轴最大分辨率 */
    int error;         /* 函数执行错误码 */

    /* 批量读取芯片配置数据，此处读取0x8047~0x8100，配置信息寄存器全部读取完 */
    error = regmap_bulk_read(ts->regmap, ts->chip->config_addr,
                            config, ts->chip->config_len);
    if (error) {  /* 读取失败则使用默认参数 */
        dev_warn(&ts->client->dev, "Error reading config: %d\n",
            error);
        ts->int_trigger_type = GOODIX_INT_TRIGGER;  /* 默认中断触发类型 */
        ts->max_touch_num = GOODIX_MAX_CONTACTS;    /* 默认最大触摸点 */
        return;
    }
    /* 从配置区提取中断触发类型，0x804D低2位 */
    ts->int_trigger_type = config[TRIGGER_LOC] & 0x03;
    /* 从配置区提取最大触摸点数量，0x804C低4位 */
    ts->max_touch_num = config[MAX_CONTACTS_LOC] & 0x0f;
    /* 小端模式读取X轴坐标输出最大值，0x8048~0x8049 */
    x_max = get_unaligned_le16(&config[RESOLUTION_LOC]);
    /* 小端模式读取Y轴坐标输出最大值，0x804A~0x804B */
    y_max = get_unaligned_le16(&config[RESOLUTION_LOC + 2]);
    if (x_max && y_max) {  /* 坐标输出最大值有效则设置输入子系统坐标范围 */
        input_abs_set_max(ts->input_dev, ABS_MT_POSITION_X, x_max - 1);
        input_abs_set_max(ts->input_dev, ABS_MT_POSITION_Y, y_max - 1);
    }
}

/*
* 读取芯片ID与固件版本信息
* @ts: 驱动核心数据结构体指针
* 返回值: 0成功，负数为错误码
*/
static int goodix_read_version(struct goodix_ts_data *ts)
{
    int error;          /* 函数执行错误码 */
    u8 buf[6];          /* 版本数据缓冲区 */
    char id_str[5];     /* 芯片ID字符串缓冲区，4字节ID + 结束符 */

    /* 读取ID与版本寄存器数据，0x8140~0x8145 */
    error = regmap_bulk_read(ts->regmap, GOODIX_REG_ID, buf, sizeof(buf));
    if (error) {
        dev_err(&ts->client->dev, "Read version failed: %d\n", error);
        return error;
    }

    /* 复制前4字节为ID字符串，0x8140~0x8143 */
    memcpy(id_str, buf, 4);
    id_str[4] = 0;  /* 添加字符串结束符 */

    /* 将ID字符串转换为16位无符号整型 */
    if (kstrtou16(id_str, 10, &ts->id)) {
        ts->id = 0x1001;  /* 转换失败则使用默认ID */
    }

    /* 小端模式读取固件版本号，0x8144~0x8145 */
    ts->version = get_unaligned_le16(&buf[4]);

    /* 打印芯片ID与版本信息 */
    dev_info(&ts->client->dev, "ID %d, version: %04x\n", ts->id, ts->version);
    
    return 0;
}

/*
* I2C通信连通性测试
* @ts: 驱动核心数据结构体指针
* 返回值: 0成功，负数为错误码
*/
static int goodix_i2c_test(struct goodix_ts_data *ts)
{
    int retry = 0;     /* 重试次数计数器 */
    int error;         /* 函数执行错误码 */
    unsigned int test; /* 测试读取的寄存器值 */

    /* 最多重试2次I2C读取 */
    while (retry++ < 2) {
        /* 读取0x8140芯片ID寄存器，测试通信 */
        error = regmap_read(ts->regmap, GOODIX_REG_ID, &test);
        if (!error)
            return 0;
        
        /* 读取失败则打印错误并延时重试 */
        dev_err(&ts->client->dev, "I2C test failed attempt %d: %d\n",
                retry, error);
        msleep(20);  /* 延时20ms后重试 */
    }
    return error;
}

/*
* 配置输入设备与中断参数
* @ts: 驱动核心数据结构体指针
* 返回值: 0成功，负数为错误码
*/
static int goodix_configure_dev(struct goodix_ts_data *ts)
{
    /* 函数执行错误码 */
    int error;

    /* 初始化默认参数 */
    ts->int_trigger_type = GOODIX_INT_TRIGGER;  /* 触发模式索引 */
    ts->max_touch_num = GOODIX_MAX_CONTACTS;    /* 默认最大触摸点 */

    /* 分配设备树管理的输入设备结构体 */
    ts->input_dev = devm_input_allocate_device(&ts->client->dev);
    if (!ts->input_dev) {
        dev_err(&ts->client->dev, "Failed to allocate input device.");
        return -ENOMEM;
    }

    /* 设置输入设备基本信息 */
    ts->input_dev->name = "Goodix Capacitive TouchScreen";  /* 设备名称 */
    ts->input_dev->phys = "input/ts";                       /* 设备物理描述 */
    ts->input_dev->id.bustype = BUS_I2C;                    /* 总线类型：I2C */
    ts->input_dev->id.vendor = 0x0416;                      /* 厂商ID */
    ts->input_dev->id.product = ts->id;                     /* 芯片ID */
    ts->input_dev->id.version = ts->version;                /* 设备版本 */

    /* 设置输入设备支持的事件：多点触摸X/Y坐标 */
    input_set_capability(ts->input_dev, EV_ABS, ABS_MT_POSITION_X);
    input_set_capability(ts->input_dev, EV_ABS, ABS_MT_POSITION_Y);
    /* 设置触摸宽度绝对参数范围 */
    input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
    /* 设置触摸压力绝对参数范围 */
    input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);

    /* 读取芯片硬件配置参数 */
    goodix_read_config(ts);
    /* 解析触摸屏物理属性，方向、翻转等 */
    touchscreen_parse_properties(ts->input_dev, true, &ts->prop);

    /* 配置无效则使用默认分辨率 */
    if (!ts->prop.max_x || !ts->prop.max_y || !ts->max_touch_num) {
        dev_err(&ts->client->dev, "Invalid config, using defaults\n");
        ts->prop.max_x = GOODIX_MAX_WIDTH - 1;    /* 默认X轴最大坐标 */
        ts->prop.max_y = GOODIX_MAX_HEIGHT - 1;   /* 默认Y轴最大坐标 */
        /* 设置输入子系统坐标范围 */
        input_abs_set_max(ts->input_dev, ABS_MT_POSITION_X, ts->prop.max_x);
        input_abs_set_max(ts->input_dev, ABS_MT_POSITION_Y, ts->prop.max_y);
    }

    /* 初始化多点触摸槽位，支持直接上报与丢弃未使用槽位 */
    error = input_mt_init_slots(ts->input_dev, ts->max_touch_num,
                                INPUT_MT_DIRECT | INPUT_MT_DROP_UNUSED);
    if (error) {
        dev_err(&ts->client->dev, "Failed to initialize MT slots: %d", error);
        return error;
    }

    /* 注册输入设备到系统输入子系统 */
    error = input_register_device(ts->input_dev);
    if (error) {
        dev_err(&ts->client->dev, "Failed to register input device: %d", error);
        return error;
    }

    /* 组合中断标志：触发类型 + 单次触发模式 */
    ts->irq_flags = goodix_irq_flags[ts->int_trigger_type] | IRQF_ONESHOT;

    /* 申请中断资源 */
    error = goodix_request_irq(ts);
    if (error) {
        dev_err(&ts->client->dev, "Request IRQ failed: %d\n", error);
        return error;
    }
    return 0;
}

/*
* I2C驱动探测函数
* @client: I2C客户端设备指针
* @id: I2C设备ID表项
* 返回值: 0成功，负数为错误码
*/
static int goodix_ts_probe(struct i2c_client *client,
                        const struct i2c_device_id *id)
{
    /* 驱动核心数据结构体指针 */
    struct goodix_ts_data *ts;
    /* 函数执行错误码 */
    int error;

    /* 分配设备树管理的核心数据内存 */
    ts = devm_kzalloc(&client->dev, sizeof(*ts), GFP_KERNEL);
    if (!ts)
        return -ENOMEM;

    /* 绑定I2C客户端设备 */
    ts->client = client;
    /* 设置I2C客户端私有数据 */
    i2c_set_clientdata(client, ts);

    /* 初始化I2C设备的Regmap句柄 */
    ts->regmap = devm_regmap_init_i2c(client, &goodix_regmap_config);
    if (IS_ERR(ts->regmap)) {
        dev_err(&client->dev, "Regmap init failed: %ld\n", PTR_ERR(ts->regmap));
        return PTR_ERR(ts->regmap);
    }

    /* 获取中断、复位引脚 */
    error = goodix_get_gpio_config(ts);
    if (error)
        return error;

    /* 存在GPIO引脚则执行芯片硬件复位并设置通信地址 */
    if (ts->gpiod_int && ts->gpiod_rst) {
        error = goodix_reset(ts);
        if (error) {
            dev_err(&client->dev, "Controller reset failed.\n");
            return error;
        }
    }

    /* 测试I2C通信是否正常 */
    error = goodix_i2c_test(ts);
    if (error) {
        dev_err(&client->dev, "I2C communication failure: %d\n", error);
        return error;
    }

    /* 读取芯片ID与版本信息 */
    error = goodix_read_version(ts);
    if (error) {
        dev_err(&client->dev, "Read version failed.\n");
        return error;
    }

    /* 根据芯片ID获取对应配置参数 */
    ts->chip = goodix_get_chip_data(ts->id);
    if (!ts->chip) {
        dev_err(&client->dev, "Unsupported chip ID: %d\n", ts->id);
        return -ENODEV;
    }

    /* 配置输入设备与中断 */
    error = goodix_configure_dev(ts);
    if (error)
        return error;

    return 0;
}

/*
* I2C驱动移除函数
* @client: I2C客户端设备指针
* 返回值: 0成功
*/
static int goodix_ts_remove(struct i2c_client *client)
{
    /* 获取I2C客户端私有数据 */
    struct goodix_ts_data *ts = i2c_get_clientdata(client);

    /* 存在GPIO引脚则释放中断资源 */
    if (ts->gpiod_int && ts->gpiod_rst) {
        goodix_free_irq(ts);
    }
    return 0;
}

/* 定义设备树匹配表 */
static const struct of_device_id goodix_of_match[] = {
    { .compatible = "fire,GT911" },
    { .compatible = "fire,GT928" },
    { }
};

/* 声明设备树匹配表，供内核自动匹配设备 */
MODULE_DEVICE_TABLE(of, goodix_of_match);

/* I2C驱动结构体定义，注册驱动到I2C总线 */
static struct i2c_driver goodix_ts_driver = {
    .probe = goodix_ts_probe,
    .remove = goodix_ts_remove,
    .driver = {
        .name = "goodix",
        .of_match_table = of_match_ptr(goodix_of_match),
    },
};

/* 简化模块入口/出口：注册/注销I2C驱动 */
module_i2c_driver(goodix_ts_driver);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("Goodix Touchscreen Driver module");
MODULE_LICENSE("GPL v2");