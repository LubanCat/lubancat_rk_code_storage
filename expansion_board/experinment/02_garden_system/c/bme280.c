/*
*
*   file: bme280.c
*   update: 2024-08-23
*   function: 适用于spi接口的bme280（兼容bmp280）
*   
*/

#include "bme280.h"

struct gpiod_chip *cs_gpiochip6;        
struct gpiod_line *cs_gpioline11;

unsigned char id;
struct bme280_parameter bme280;

int fd_spidev;

int init_flag = 0;

/*****************************
 * @brief : spi接口初始化
 * @param : none
 * @return: 0初始化成功 -1初始化失败
*****************************/
static int spi_init(void)
{
    int ret; 
    SPI_MODE mode;
    char spi_bits;
    SPI_SPEED spi_speed;

    fd_spidev = open(BME280_SPI_DEV, O_RDWR);
	if (fd_spidev < 0) {
		printf("open %s err\n", BME280_SPI_DEV);
		return -1;
	}

    /* mode */
    mode = SPIMODE0;
    ret = ioctl(fd_spidev, SPI_IOC_WR_MODE, &mode);                //mode 0
    if (ret < 0) {
		printf("SPI_IOC_WR_MODE err\n");
		return -1;
	}

    /* bits per word */
    spi_bits = 8;
    ret = ioctl(fd_spidev, SPI_IOC_WR_BITS_PER_WORD, &spi_bits);   //8bits 
    if (ret < 0) {
		printf("SPI_IOC_WR_BITS_PER_WORD err\n");
		return -1;
	}

    /* speed */
    spi_speed = (uint32_t)S_1M;
    ret = ioctl(fd_spidev, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed);    //1MHz    
    if (ret < 0) {
		printf("SPI_IOC_WR_MAX_SPEED_HZ err\n");
		return -1;
	}
    
    /* cs pin init */
    cs_gpiochip6 = gpiod_chip_open("/dev/gpiochip6");
    if(cs_gpiochip6 == NULL)
    {
        printf("gpiod_chip_open error\n");
        return -1;
    }
    cs_gpioline11 = gpiod_chip_get_line(cs_gpiochip6, 11);
    if(cs_gpioline11 == NULL)
    {
        printf("gpiod_chip_get_line error\n");
        return -1;
    }
    ret = gpiod_line_request_output(cs_gpioline11, "cs_gpioline11", 1);
    if(ret < 0)
    {
        printf("gpiod_line_request_output error : cs_gpioline11\n");
        return -1;
    }
}

/*****************************
 * @brief : bme280写寄存器
 * @param : reg 目标寄存器
 * @param : byte 要往目标寄存器写入的值
 * @return: none
*****************************/
static void bme280_write_reg(const unsigned char reg, const unsigned char byte)
{
    unsigned char reg_buf[1];
	unsigned char byte_buf[1];

    memset(reg_buf, reg, sizeof(reg_buf));
	memset(byte_buf, byte, sizeof(byte_buf));

    gpiod_line_set_value(cs_gpioline11, 0);

    write(fd_spidev, &reg_buf[0], 1);
    write(fd_spidev, &byte_buf[0], 1);
	
    gpiod_line_set_value(cs_gpioline11, 1);
}

/*****************************
 * @brief : bme280读寄存器
 * @param : reg 目标寄存器
 * @return: 返回从目标寄存器读到的值
*****************************/
static unsigned char bme280_read_reg(const unsigned char reg)
{
    struct spi_ioc_transfer	xfer[2];
    unsigned char reg_buf[1];
	unsigned char recv_buf[1];
	int status;

    memset(xfer, 0, sizeof(xfer));
    memset(reg_buf, reg, sizeof(reg_buf));
	memset(recv_buf, 0, sizeof(recv_buf));

	xfer[0].tx_buf = (unsigned long)reg_buf;
	xfer[0].len = 1;

	xfer[1].rx_buf = (unsigned long)recv_buf;
	xfer[1].len = 1;

    gpiod_line_set_value(cs_gpioline11, 0);

	status = ioctl(fd_spidev, SPI_IOC_MESSAGE(2), xfer);
	if (status < 0) {
		perror("SPI_IOC_MESSAGE");
		return 0;
	}

    gpiod_line_set_value(cs_gpioline11, 1);

	return recv_buf[0];
}

/*****************************
 * @brief : bme280读修正参数
 * @param : none
 * @return: none
*****************************/
static void bme280_read_fix_parameter(void)
{
	unsigned char tmp;

	//dig_T1
    bme280.T1 = bme280_read_reg(0x89);
    bme280.T1 <<= 8;
    bme280.T1 |= bme280_read_reg(0x88);

    //dig_T2
    bme280.T2 = bme280_read_reg(0x8B);
    bme280.T2 <<= 8;
    bme280.T2 |= bme280_read_reg(0x8A);

    //dig_T3
    bme280.T3 = bme280_read_reg(0x8D);
    bme280.T3 <<= 8;
    bme280.T3 |= bme280_read_reg(0x8C);

    //dig_P1
    bme280.P1 = bme280_read_reg(0x8F);
    bme280.P1 <<= 8;
    bme280.P1 |= bme280_read_reg(0x8E);

    //dig_P2
    bme280.P2 = bme280_read_reg(0x91);
    bme280.P2 <<= 8;
    bme280.P2 |= bme280_read_reg(0x90);

    //dig_P3
    bme280.P3 = bme280_read_reg(0x93);
    bme280.P3 <<= 8;
    bme280.P3 |= bme280_read_reg(0x92);

    //dig_P4
    bme280.P4 = bme280_read_reg(0x95);
    bme280.P4 <<= 8;
    bme280.P4 |= bme280_read_reg(0x94);

    //dig_P5
    bme280.P5 = bme280_read_reg(0x97);
    bme280.P5 <<= 8;
    bme280.P5 |= bme280_read_reg(0x96);

    //dig_P6
    bme280.P6 = bme280_read_reg(0x99);
    bme280.P6 <<= 8;
    bme280.P6 |= bme280_read_reg(0x98);

    //dig_P7
    bme280.P7 = bme280_read_reg(0x9B);
    bme280.P7 <<= 8;
    bme280.P7 |= bme280_read_reg(0x9A);

    //dig_P8
    bme280.P8 = bme280_read_reg(0x9D);
    bme280.P8 <<= 8;
    bme280.P8 |= bme280_read_reg(0x9C);

    //dig_P9
    bme280.P9 = bme280_read_reg(0x9F);
    bme280.P9 <<= 8;
    bme280.P9 |= bme280_read_reg(0x9E);

    //dig_H1
    bme280.H1 = bme280_read_reg(0xA1);

    //dig_H2
    bme280.H2 = bme280_read_reg(0xE2);
    bme280.H2 <<= 8;
    bme280.H2 |= bme280_read_reg(0xE1);

    //dig_H3
    bme280.H3 = bme280_read_reg(0xE3);

    //dig_H4
    bme280.H4 = bme280_read_reg(0xE4);
    bme280.H4 <<= 4;
    tmp = bme280_read_reg(0xE5);
    tmp &= 0x0f;
    bme280.H4 |= tmp;

    //dig_H
    bme280.H5 = bme280_read_reg(0xE6);
    bme280.H5 <<= 4;
    tmp = bme280_read_reg(0xE5);
    tmp &= 0xf0;
    tmp >>= 4;
    bme280.H5 |= tmp;

    //dig_H6
    bme280.H6 = bme280_read_reg(0xE7);
}

/*****************************
 * @brief : bme280读温度值
 * @param : none
 * @return: none
*****************************/
static void bme280_read_temp_parameter(void)
{
	int ret;

    // read temp data
    ret = bme280_read_reg(BME280_REGISTER_TEMP_MSB);
    bme280.adc_T = ret;
    bme280.adc_T <<= 8;

    ret = bme280_read_reg(BME280_REGISTER_TEMP_LSB);
    bme280.adc_T |= ret;
    bme280.adc_T <<= 8;

    ret = bme280_read_reg(BME280_REGISTER_TEMP_XLSB);
    bme280.adc_T |= ret;
    bme280.adc_T >>= 4;
}

/*****************************
 * @brief : bme280读压力值
 * @param : none
 * @return: none
*****************************/
static void bme280_read_pres_parameter(void)
{
	int ret;

    // read pressure data
    ret = bme280_read_reg(BME280_REGISTER_PRESS_MSB);
	bme280.adc_P = ret;
	bme280.adc_P <<= 8;

	ret = bme280_read_reg(BME280_REGISTER_PRESS_LSB);
	bme280.adc_P |= ret;
	bme280.adc_P <<= 8;

	ret = bme280_read_reg(BME280_REGISTER_PRESS_XLSB);
	bme280.adc_P |= ret;
	bme280.adc_P >>= 4;
}

/*****************************
 * @brief : bme280读湿度值
 * @param : none
 * @return: none
*****************************/
static void bme280_read_humi_parameter(void)
{
	int ret;

    // read humi data
	ret = bme280_read_reg(BME280_REGISTER_HUMI_MSB);
	bme280.adc_H = ret;
	bme280.adc_H <<= 8;

	ret = bme280_read_reg(BME280_REGISTER_HUMI_LSB);
	bme280.adc_H |= ret;
}

/*****************************
 * @brief : bme280重新测量环境数据
 * @param : none
 * @return: none
*****************************/
static void bme280_remeasure(void)
{
    bme280_write_reg(BME280_REGISTER_CTRL_MEAS, 0x55);
    usleep(45000);
}

/*****************************
 * @brief : bme280根据修正参数和温度值计算最终的温度数据
 * @param : none
 * @return: 返回温度数据
*****************************/
static float bme280_compute_temp(void)
{
    int var1, var2, T;

    var1 = ((((bme280.adc_T>>3) - (bme280.T1<<1))) * bme280.T2) >> 11;
    var2 = (((((bme280.adc_T>>4)-bme280.T1) * (bme280.adc_T>>4)-bme280.T1) >> 12) * bme280.T3) >> 14;

    bme280.t_fine = var1 + var2;

    T = (bme280.t_fine * 5 + 128) >> 8;
    return (float)T/100;
}

/*****************************
 * @brief : bme280根据修正参数和压力值计算最终的压力数据
 * @param : none
 * @return: 返回压力数据
*****************************/
static float bme280_compute_pres(void)
{
    int64_t var1, var2, p;

    var1 = ((int64_t)bme280.t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)bme280.P6;
    var2 = var2 + ((var1 * (int64_t)bme280.P5) << 17);
    var2 = var2 + (((int64_t)bme280.P4) << 35);
    var1 = ((var1 * var1 * (int64_t)bme280.P3) >> 8) +((var1 * (int64_t)bme280.P2) << 12);
    var1 =(((((int64_t)1) << 47) + var1)) * ((int64_t)bme280.P1) >> 33;

    if (var1 == 0)
    {
        return 0;
    }
    else
    {
        p = 1048576 - bme280.adc_P;
        p = (((p << 31) - var2) * 3125) / var1;
        var1 = (((int64_t)bme280.P9) * (p >> 13) * (p >> 13)) >> 25;
        var2 = (((int64_t)bme280.P8) * p) >> 19;
        p = ((p + var1 + var2) >> 8) + (((int64_t)bme280.P7) << 4);

        return (float)p/256;
    }
}

/*****************************
 * @brief : bme280根据修正参数和湿度值计算最终的湿度数据
 * @param : none
 * @return: 返回湿度数据
*****************************/
static float bme280_compute_humi(void)
{
    double var_H;

    var_H = (((double)bme280.t_fine) - 76800.00);
    var_H = (bme280.adc_H - (((double)bme280.H4) * 64.0 + ((double)bme280.H5) / 16384.0 * var_H)) * (((double)bme280.H2) / 65536.0 * (1.0 + ((double)bme280.H6) / 67108864.0 * var_H * (1.0 + ((double)bme280.H3) / 67108864.0 * var_H)));
    var_H = var_H * (1.0 - ((double)bme280.H1) * var_H / 524288.0);

    if(var_H > 100.0)
    {
        var_H = 100.0;
    }
    else if(var_H < 0.0)
    {
        var_H = 0.0;
    }

    return var_H;
}

/*****************************
 * @brief : bme280获取温度数据
 * @param : none
 * @return: 返回温度数据
*****************************/
float bme280_get_temp(void)
{
    if(!init_flag)
        return 0;

    bme280_remeasure();

    bme280_read_temp_parameter();

    return bme280_compute_temp();
}

/*****************************
 * @brief : bme280获取压力数据
 * @param : none
 * @return: 返回压力数据
*****************************/
float bme280_get_pres(void)
{
    if(!init_flag)
        return 0;

    bme280_remeasure();

    // 计算压力值需要用到温度的数据
    bme280_read_temp_parameter();
    bme280_compute_temp();

    bme280_read_pres_parameter();
    return bme280_compute_pres();
}

/*****************************
 * @brief : bme280获取湿度数据
 * @param : none
 * @return: 返回湿度数据
*****************************/
float bme280_get_humi(void)
{
    if(!init_flag || id == 0x58)
    {
        return 0;
        printf("BMP280 can not get humi data\n");
    }

    bme280_remeasure();
    
    // 计算湿度值需要用到温度的数据
    bme280_read_temp_parameter();
    bme280_compute_temp();

    bme280_read_humi_parameter();
    return bme280_compute_humi();
}

/*****************************
 * @brief : bme280初始化
 * @param : none
 * @return: 0初始化成功 -1初始化失败
*****************************/
int bme280_init(void)
{
    int ret;
    init_flag = 0;

    /* spi init */
    ret = spi_init();
    if(ret == -1)
    {
        printf("spi init err\n");
        return -1;
    }

    /* bme280 init */
    /* 16倍气压过采样，2倍温度过采样，force mode */
    bme280_write_reg(BME280_REGISTER_CTRL_MEAS, 0x55);
    /* 滤波器系数16 */
    bme280_write_reg(BME280_REGISTER_CONFIG, 0x10);
    /* 读取id，检测通信是否正常 */
    id = bme280_read_reg(BME280_REGISTER_ID);
    if(id == 0x58)
        printf("dev is BMP280, id : 0x%x\n", id);
    else if(id == 0x60)
        printf("dev is BME280, id : 0x%x\n", id);
    else
    {
        printf("can not find BME280/BMP280, read id: 0x%x\n", id);
        return -1;
    }
    
    /* 读取修正参数 */
    bme280_read_fix_parameter();

    init_flag = 1;

    return 0;
}

/*****************************
 * @brief : bme280反初始化
 * @param : none
 * @return: none
*****************************/
void bme280_exit(void)
{
    if(!init_flag)
        return;

    close(fd_spidev);

    gpiod_line_release(cs_gpioline11);
    gpiod_chip_close(cs_gpiochip6);

    init_flag = 0;
}
