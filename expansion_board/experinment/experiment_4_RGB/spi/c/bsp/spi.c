/*
*
*   file: spi.c
*   updata: 2024-12-05
*
*/

#include "spi.h"

static int fd_spidev;
static int init_flag = 0;       // 1已初始化 0未初始化
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*****************************
 * @brief : 初始化 SPI 设备
 * @param : spi_dev - SPI 设备路径
 * @return: 成功返回 0，失败返回 -1
 * @note  : 初始化 SPI 接口，配置 SPI 参数。
 *****************************/
int spi_init(const char *spi_dev)
{
    int ret; 
    SPI_MODE mode;
    char spi_bits;
    SPI_SPEED spi_speed;

    fd_spidev = open(spi_dev, O_RDWR);
	if (fd_spidev < 0) {
		printf("open %s err\n", spi_dev);
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
    spi_speed = (uint32_t)S_8M;
    ret = ioctl(fd_spidev, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed);    //1MHz    
    if (ret < 0) {
		printf("SPI_IOC_WR_MAX_SPEED_HZ err\n");
		return -1;
	}

    init_flag = 1;
    return 0;
}

/*****************************
 * @brief : 向 SPI 总线写入数据并读取数据
 * @param : send_buf     - 发送数据的缓冲区
 *          send_buf_len - 发送数据的长度
 *          recv_buf     - 接收数据的缓冲区
 *          recv_buf_len - 接收数据的长度
 * @return: 无返回值
 * @note  : 通过 SPI 总线发送和接收数据，发送的数据通过 `send_buf`，接收到的数据存放在 `recv_buf` 中。
 *****************************/
static void spi_write_then_read(unsigned char *send_buf, unsigned int send_buf_len, unsigned char *recv_buf, unsigned int recv_buf_len)
{
    struct spi_ioc_transfer	xfer[2];
	int status;

    if(init_flag == 0)
    {
        perror("spidev can not init!\n");
        return;
    }

    if(send_buf == NULL || recv_buf == NULL)
        return;

    if(send_buf_len < 1 || recv_buf_len < 1)
        return;

    memset(xfer, 0, sizeof(xfer));

	xfer[0].tx_buf = (unsigned long)send_buf;
	xfer[0].len = send_buf_len;

	xfer[1].rx_buf = (unsigned long)recv_buf;
	xfer[1].len = recv_buf_len;

	status = ioctl(fd_spidev, SPI_IOC_MESSAGE(2), xfer);
	if (status < 0) {
		perror("SPI_IOC_MESSAGE");
		return;
	}
}

/*****************************
 * @brief : 向 SPI 总线写入一个字节数据
 * @param : data - 待写入的数据字节
 * @return: 无返回值
 * @note  : 通过 SPI 总线发送一个字节的数据。
 *****************************/
static void spi_write_byte_data(unsigned char data)
{
    unsigned char buff[1] = {data};

    if(init_flag == 0)
    {
        perror("spidev can not init!\n");
        return;
    }

    write(fd_spidev, &buff, 1);
}

/*****************************
 * @brief : 向 SPI 总线写入n个字节数据
 * @param : send_buf - 待写入的数据
 * @param : send_buf_len - 待写入的数据长度
 * @return: 无返回值
 * @note  : 通过 SPI 总线发送n个字节的数据。
 *****************************/
static void spi_write_nbyte_data(unsigned char *send_buf, unsigned int send_buf_len)
{
    struct spi_ioc_transfer	xfer[2];
    unsigned char recv_buf[send_buf_len];
	int status;

    if(init_flag == 0)
    {
        perror("spidev can not init!\n");
        return;
    }

    if(send_buf == NULL || send_buf_len < 1)
        return;

    memset(xfer, 0, sizeof(xfer));
    memset(recv_buf, 0, sizeof(send_buf_len));

	xfer[0].tx_buf = (unsigned long)send_buf;
    xfer[0].rx_buf = (unsigned long)recv_buf;
	xfer[0].len = send_buf_len;

	status = ioctl(fd_spidev, SPI_IOC_MESSAGE(1), xfer);
	if (status < 0) {
		perror("SPI_IOC_MESSAGE");
		return;
	}
}

/*****************************
 * @brief : 关闭 SPI 
 * @param : none
 * @return: 无返回值
 *****************************/
void spi_exit()
{
    if(fd_spidev >= 0)
        close(fd_spidev);
    
    init_flag = 0;
}

static spi_operations_t spi_ops = {
    .spi_write_then_read = spi_write_then_read,
    .spi_write_byte_data = spi_write_byte_data,
    .spi_write_nbyte_data = spi_write_nbyte_data,
    .mutex = &mutex,
};

/*****************************
 * @brief : 获取SPI操作函数
 * @param : none
 * @return: 返回spi_operations_t结构体指针
 *****************************/
spi_operations_t *get_spi_ops()
{
    return &spi_ops;
};
