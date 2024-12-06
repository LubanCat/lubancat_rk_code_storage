/*
*
*   file: spi.h
*   updata: 2024-12-05
*
*/

#ifndef _SPI_H
#define _SPI_H

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
#include <pthread.h>

typedef struct spi_operations
{
    void (*spi_write_then_read)(unsigned char *send_buf, unsigned int send_buf_len, unsigned char *recv_buf, unsigned int recv_buf_len);
    void (*spi_write_byte_data)(unsigned char data);
	void (*spi_write_nbyte_data)(unsigned char *send_buf, unsigned int send_buf_len);
	pthread_mutex_t *mutex;
}spi_operations_t;

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
	S_8M    = 8000000,
	S_13_5M = 13500000,
	S_27M   = 27000000,
}SPI_SPEED;

int spi_init(const char *spi_dev);
void spi_exit();
spi_operations_t *get_spi_ops();

#endif