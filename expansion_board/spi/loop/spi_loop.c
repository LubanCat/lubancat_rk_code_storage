/*
*
*   file: spi_loop.c
*   update: 2024-08-13
*   usage: 
*       sudo gcc -o spi_loop spi_loop.c 
*       sudo ./spi_loop
*
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

int main(int argc, char **argv)
{
    int fd;
	unsigned int val;
	struct spi_ioc_transfer	xfer[1];
	int status;

	unsigned char tx_buf[4] = {0xaa, 0xbb, 0xcc, 0xdd};	
	unsigned char rx_buf[4];	
	
	if (argc != 2)
	{
		printf("Usage: %s /dev/spidevB.D\n", argv[0]);
		return 0;
	}

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("can not open %s\n", argv[1]);
		return 1;
	}
	
	memset(xfer, 0, sizeof(xfer));

	xfer[0].tx_buf = tx_buf;
	xfer[0].rx_buf = rx_buf;
	xfer[0].len = 4;
	
	status = ioctl(fd, SPI_IOC_MESSAGE(1), xfer);
	if (status < 0) {
		printf("SPI_IOC_MESSAGE\n");
		return -1;
	}

    printf("send buff: %x %x %x %x\n", tx_buf[0], tx_buf[1], tx_buf[2], tx_buf[3]);
	printf("receive buff: %x %x %x %x\n", rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3]);	
	
	close(fd);

	return 0;
}