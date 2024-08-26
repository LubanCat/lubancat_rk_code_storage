/*
*
*   file: ads1115.c
*   update: 2024-08-14
*   function: 适用于ADS1115
*
*/

#include "ads1115.h"

static int file;
static char filename[20];

/*****************************
 * @brief : ads1115配置
 * @param : none
 * @return: none
*****************************/
static void ads1115_config(void)
{
    // 发送两个字节数据给ADS1115（规定是MSB先行），但i2c_smbus_write_word_data()是LSB先行，所以把config_H放在低8位
    uint16_t config = (CONFIG_REG_L << 8) | CONFIG_REG_H;
    i2c_smbus_write_word_data(file, ADS1115_REG_POINTER_CONFIG, config);
}

/*****************************
 * @brief : ads1115读取电压值
 * @param : none
 * @return: 返回当前配置端口的电压值
*****************************/
double ads1115_read_vol(void)
{
    uint16_t ad_val;
    double vol = 0;
    double vol_range = 0;

    // 因为配置的是单次触发，所以读数据前需要触发单次转换
    ads1115_config();
    usleep(100000);

    // 读ADS1115两个字节数据（ADS1115是先发MSB），但i2c_smbus_read_word_data()是先把收到的8bit数据当成LSB，收到数据后，需要将LSB和MSB调转
    ad_val = i2c_smbus_read_word_data(file, ADS1115_REG_POINTER_CONVERT);
    ad_val = (ad_val & 0x00FF) << 8 | (ad_val & 0xFF00) >> 8;

    // 超出量程范围
    if((ad_val==0x7FFF) | (ad_val==0X8000))                 
    {
        printf("ad_val is over range!\n");
        return 0;
    }

    switch((0x0E & CONFIG_REG_H) >> 1)                
    {
        case(0x00):
            vol_range = 6.1144;
            break;
        case(0x01):
            vol_range = 4.096;
            break;
        case(0x02):
            vol_range = 2.048;
            break;
        case(0x03):
            vol_range = 1.024;
            break;
        case(0x04):
            vol_range = 0.512;
            break;
        case(0x05):
            vol_range = 0.256;
            break;
        default:
            printf("into switch\n");
            break;
    }

    if (ad_val >= 0x8000)    
        vol = ((double)(0xFFFF - ad_val) / 32768.0) * vol_range;    // 反转部分的处理  
    else 
        vol = ((double)ad_val / 32768.0) * vol_range;               // 正常部分的处理  
    
    return vol;
}

/*****************************
 * @brief : ads1115初始化
 * @param : none
 * @return: 0初始化成功 -1初始化失败
*****************************/
int ads1115_init(void)
{
    file = open_i2c_dev(ADS1115_I2C_BUS, filename, sizeof(filename), 0);
	if (file < 0)
	{
		printf("can't open %s\n", filename);
		return -1;
	}

    if (set_slave_addr(file, ADS1115_I2C_DEV, 1))
	{
		printf("%s: can't set_slave_addr\n", __FUNCTION__);
		return -1;
	}
}

/*****************************
 * @brief : ads1115反初始化
 * @param : none
 * @return: none
*****************************/
void ads1115_exit(void)
{
    if(file >= 0)
        close(file);
}
