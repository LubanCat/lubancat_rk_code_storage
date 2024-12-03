/*
*
*   file: heartrate.c
*   update: 2024-10-25
*
*/

#include "max30102.h"

// 文件描述符和文件名
static int file;
static char filename[20];

// 用于存储滤波数据的缓冲区
static uint32_t data_buffer_0[FILTER_SIZE] = {0};   // 存储data[0]的缓冲区（IR信号）
static uint32_t data_buffer_1[FILTER_SIZE] = {0};   // 存储data[1]的缓冲区（RED信号）
static int index_0 = 0;                             // data[0]的索引
static int index_1 = 0;                             // data[1]的索引

/*****************************
 * @brief : max30102写寄存器
 * @param : reg 寄存器地址
 * @param : cmd 要写入寄存器的值
 * @return: none
*****************************/
static void max30102_write_reg(uint8_t reg, uint8_t cmd)
{
    i2c_smbus_write_byte_data(file, reg, cmd);
}

/*****************************
 * @brief : max30102读寄存器（读1个字节）
 * @param : reg 寄存器地址
 * @return: 返回读到的值
*****************************/
static uint8_t max30102_read_reg(uint8_t reg)
{
    return i2c_smbus_read_byte_data(file, reg);
}

/*****************************
 * @brief : max30102读寄存器（读n个字节）
 * @param : reg 寄存器地址
 * @param : buff 存储读取数据的缓冲区
 * @param : len 要读取的字节长度 
 * @return: 返回读到的值
*****************************/
static void max30102_read_reg_n(uint8_t reg, uint8_t *buff, uint8_t len)
{
    i2c_smbus_read_i2c_block_data(file, reg, len, buff);
}

/*****************************
 * @brief : max30102初始化
 * @param : i2c_bus i2c总线编号
 * @return: 0成功 -1失败
*****************************/
int max30102_init(uint8_t i2c_bus)
{
    uint8_t id;

    file = open_i2c_dev(i2c_bus, filename, sizeof(filename), 0);
	if (file < 0)
	{
		printf("can't open %s\n", filename);
		return -1;
	}

	if (set_slave_addr(file, MAX30102_ADDRESS, 1))
	{
		printf("can't set_slave_addr\n");
		return -1;
	}

    // 读取ID，检查I2C通信是否正常
    id = max30102_read_reg(MAX30102_REG_PARTID);
    if(id != 0x15)
    {
        printf("can not find max30102!\n");
        return -1;
    }
    printf("max30102 id : 0x%X\n", id);

    max30102_write_reg(MAX30102_REG_MODE_CONF, 0x40);           // 复位设备        
    usleep(50000);

    max30102_write_reg(MAX30102_REG_INTR_ENABLE1, 0xE0);        // 使能中断：FIFO几乎满标志、新FIFO数据准备就绪、环境光消除溢出、供电准备就绪
    max30102_write_reg(MAX30102_REG_INTR_ENABLE2, 0x00);        // 使能内部温度标准标志

    max30102_write_reg(MAX30102_REG_FIFO_WRITE_PTR, 0x00);      // 清除FIFO写指针寄存器
    max30102_write_reg(MAX30102_REG_OVERFLOW_COUNTER, 0x00);    // 清除FIFO溢出计数器，重置为0
    max30102_write_reg(MAX30102_REG_FIFO_READ_PTR, 0x00);       // 清除FIFO读指针寄存器

    max30102_write_reg(MAX30102_REG_FIFO_CONF, 0x4F);           // FIFO配置：样本平均值(4)，FIFO满时循环(0)，FIFO几乎满值(当剩余15个空样本时产生中断)

    max30102_write_reg(MAX30102_REG_MODE_CONF, 0x03);           // 设置为SpO2模式

    max30102_write_reg(MAX30102_REG_SPO2_CONF, 0x2A);           // SpO2配置：电流15.63pA，采样率200Hz，LED脉宽215us

    max30102_write_reg(MAX30102_REG_LED1_PA, 0x2F);             // IR LED电流
    max30102_write_reg(MAX30102_REG_LED2_PA, 0x2F);             // RED LED电流

    max30102_write_reg(MAX30102_REG_DIE_TEMP_CONFIG, 0x01);     // 配置温度传感器，设置为自动清除

    max30102_read_reg(MAX30102_REG_INTR_STATUS1);               // 清除中断状态
    max30102_read_reg(MAX30102_REG_INTR_STATUS2);               // 清除中断状态

    return 0;
}

/*****************************
 * @brief : 从FIFO读取数据
 * @param : data 存储读取数据的缓冲区
 * @return: none
*****************************/
void max30102_read_fifo(uint32_t *data)
{
    uint8_t receive_data[6], temp_data=0;

    if(file < 0)
        return;
    
    if(data == NULL)
        return;

    temp_data = max30102_read_reg(MAX30102_REG_INTR_STATUS1);                               // 读取中断状态寄存器
    while((temp_data & 0x40) != 0x40)                                                       // 等待FIFO中有数据
    {
        temp_data = max30102_read_reg(MAX30102_REG_INTR_STATUS1);
    }

    max30102_read_reg_n(MAX30102_REG_FIFO_DATA, receive_data, 6);                           // 读取FIFO数据 
    data[0] = (receive_data[0]<<16 | receive_data[1]<<8 | receive_data[2]) & 0x03ffff;      // 解析IR数据
    data[1] = (receive_data[3]<<16 | receive_data[4]<<8 | receive_data[5]) & 0x03ffff;      // 解析RED数据
}

/***************************** 
 * @brief : 移动均值滤波 
 * @param : new_data_0 新数据点data[0]（IR信号）
 * @param : new_data_1 新数据点data[1]（RED信号）
 * @param : avg_data_0 返回的平均值data[0]
 * @param : avg_data_1 返回的平均值data[1]
 * @return: none
*****************************/
void max30102_average_filter(uint32_t new_data_0, uint32_t new_data_1, uint32_t *avg_data_0, uint32_t *avg_data_1)
{
    uint32_t sum_0 = 0, sum_1 = 0;

    // 更新data[0]的数据缓冲区
    data_buffer_0[index_0] = new_data_0;
    index_0 = (index_0 + 1) % FILTER_SIZE;  // 环形缓冲区

    // 更新data[1]的数据缓冲区
    data_buffer_1[index_1] = new_data_1;
    index_1 = (index_1 + 1) % FILTER_SIZE;  // 环形缓冲区

    // 计算data[0]的移动平均
    for (int i = 0; i < FILTER_SIZE; i++) 
        sum_0 += data_buffer_0[i];
    *avg_data_0 = sum_0 / FILTER_SIZE;      // 返回data[0]的平均值

    // 计算data[0]的移动平均
    for (int i = 0; i < FILTER_SIZE; i++) 
        sum_1 += data_buffer_1[i];
    *avg_data_1 = sum_1 / FILTER_SIZE;      // 返回data[1]的平均值
}

/*****************************
 * @brief : 计算心率
 * @param : input_data 输入数据数组
 * @param : cache_nums 缓存的样本数
 * @return: 返回计算得到的心率
*****************************/
uint16_t max30102_getHeartRate(float *input_data, uint16_t cache_nums)
{
    float input_data_sum_aver = 0;          // 存储输入数据的平均值
    uint16_t i, temp;                       // temp用于存储两个相邻心跳峰值之间的样本数

    // 计算输入数据的平均值
    for(i = 0; i < cache_nums; i++)
    {
        input_data_sum_aver += *(input_data + i);
    }
    input_data_sum_aver = input_data_sum_aver / cache_nums;

    // 查找第一个峰值
    for(i = 0; i < cache_nums; i++)
    {
        if((*(input_data+i) > input_data_sum_aver) && (*(input_data+i+1) < input_data_sum_aver))
        {
            temp = i;                   // 记录峰值的索引
            break;
        }
    }
    i++;
    
    // 查找第二个峰值
    for(; i < cache_nums; i++)
    {
        if((*(input_data+i) > input_data_sum_aver) && (*(input_data+i+1) < input_data_sum_aver))
        {
            temp = i - temp;            // 计算两个相邻心跳峰值之间的样本数
            break;
        }
    }

    // 根据样本数计算心率
    if((temp > 14) && (temp < 100))     // 限制样本数范围
    {
        // 心率 = (采样频率 * 60) / temp
        // 采样频率表示每秒钟所采样的样本数，乘以60表示1分钟所采的样本数
        // temp表示两个相邻心跳峰值之间的样本数
        return ((200/4) * 60) / temp;       // 其中SpO2采样频率配置为了200Hz，FIFO平均过滤值配置为了4，所以有效的采样率为 200Hz/4=50Hz
    }
    else
    {
        return 0;
    }
}

/*****************************
 * @brief : 计算SpO2
 * @param : ir_input_data IR输入数据
 * @param : red_input_data RED输入数据
 * @param : cache_nums 缓存的样本数
 * @return: 返回计算得到的SpO2值
*****************************/
float max30102_getSpO2(float *ir_input_data,float *red_input_data,uint16_t cache_nums)
{
    float ir_max = *ir_input_data, ir_min = *ir_input_data;
    float red_max = *red_input_data, red_min = *red_input_data;
    float R;
    uint16_t i;

    // 计算IR和RED的最大最小值
    for(i = 1; i < cache_nums; i++)
    {
        if(ir_max < *(ir_input_data+i))
        {
            ir_max = *(ir_input_data+i);
        }
        if(ir_min > *(ir_input_data+i))
        {
            ir_min = *(ir_input_data+i);
        }
        if(red_max < *(red_input_data+i))
        {
            red_max = *(red_input_data+i);
        }
        if(red_min > *(red_input_data+i))
        {
            red_min = *(red_input_data+i);
        }
    }
    // 计算比值R
    R = ((ir_max - ir_min) * red_min) / ((red_max - red_min) * ir_min);
    return ((-45.060) * R * R + 30.354 * R + 94.845);  // 根据比值R计算SpO2值
}

/*****************************
 * @brief : max30102反初始化
 * @param : none
 * @return: none
*****************************/
void max30102_exit()
{
    if(file >= 0)
        close(file);
}