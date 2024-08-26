/*
*
*   file: bme280.h
*   update: 2024-08-23
*
*/

#ifndef _BME280_H
#define _BME280_H

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <gpiod.h>
#include <stdint.h>
#include <linux/spi/spidev.h>

#define BME280_SPI_DEV                  "/dev/spidev3.0"

/* BME280 REGISTER */
#define BME280_REGISTER_ID              (0xD0)
#define BME280_REGISTER_RESET           (0xE0)
#define BME280_REGISTER_STATUS          (0xF3)
#define BME280_REGISTER_CTRL_MEAS       (0xF4)
#define BME280_REGISTER_CONFIG          (0xF5)
#define BME280_REGISTER_PRESS_MSB       (0xF7)
#define BME280_REGISTER_PRESS_LSB       (0xF8)
#define BME280_REGISTER_PRESS_XLSB      (0xF9)
#define BME280_REGISTER_TEMP_MSB        (0xFA)
#define BME280_REGISTER_TEMP_LSB        (0xFB)
#define BME280_REGISTER_TEMP_XLSB       (0xFC)
#define BME280_REGISTER_HUMI_MSB        (0xFD)
#define BME280_REGISTER_HUMI_LSB        (0xFE)

typedef enum
{
	SPIMODE0 = SPI_MODE_0,
	SPIMODE1 = SPI_MODE_1,
	SPIMODE2 = SPI_MODE_2,
	SPIMODE3 = SPI_MODE_3,
}SPI_MODE;
 
typedef enum
{
    S_1M    = 1000000,
	S_6_75M = 6750000,
	S_13_5M = 13500000,
	S_27M   = 27000000,
}SPI_SPEED;

struct bme280_parameter{
    unsigned short int T1;
    short int T2;
    short int T3;
    unsigned short int P1;
    short int P2;
    short int P3;
    short int P4;
    short int P5;
    short int P6;
    short int P7;
    short int P8;
    short int P9;
    unsigned char H1;
    short int H2;
    unsigned char H3;
    short int H4;
    short int H5;
    unsigned char H6;
    int adc_T;
    int adc_P;
    int adc_H;
    int t_fine;
};

int bme280_init(void);
void bme280_exit(void);
float bme280_get_temp(void);
float bme280_get_pres(void);
float bme280_get_humi(void);

#endif