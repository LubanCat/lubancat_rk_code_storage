/*
*
*   file: w25qxx.c
*   updata: 2024-11-11
*
*/

#include "w25qxx.h"

struct gpiod_chip *cs_gpiochip;        
struct gpiod_line *cs_gpioline;

spi_operations_t *w25qxx_spi_ops = NULL;

/*****************************
 * @brief : 初始化 W25QXX 芯片的 CS 引脚
 * @param : 无
 * @return: 成功返回0 失败返回-1
 *****************************/
static int w25qxx_cs_pin_init(const char *cs_chip, unsigned int cs_pin)
{
    int ret; 

    /* cs pin init */
    cs_gpiochip = gpiod_chip_open(cs_chip);
    if(cs_gpiochip == NULL)
    {
        printf("gpiod_chip_open error\n");
        return -1;
    }
    cs_gpioline = gpiod_chip_get_line(cs_gpiochip, cs_pin);
    if(cs_gpioline == NULL)
    {
        printf("gpiod_chip_get_line error\n");
        return -1;
    }
    ret = gpiod_line_request_output(cs_gpioline, "cs_gpioline", 1);
    if(ret < 0)
    {
        printf("gpiod_line_request_output error : cs_gpioline\n");
        return -1;
    }

    return 0;
}

/*****************************
 * @brief : 读取 W25QXX 芯片的 ID
 * @param : 无
 * @return: 返回 W25QXX 芯片的设备 ID
 *****************************/
static unsigned int w25qxx_read_id()
{
    unsigned char send_buf[4] = {0x00, 0x00, 0x00, 0x00};   
    unsigned char recv_buf[2] = {0};
    unsigned int id;

    if(w25qxx_spi_ops == NULL)
        return -1;

    send_buf[0] = W25QXX_MANUFACTURER_DEVICE_ID;            // Manufacturer/Device ID

    gpiod_line_set_value(cs_gpioline, 0);

    pthread_mutex_lock(w25qxx_spi_ops->mutex);
    w25qxx_spi_ops->spi_write_then_read(send_buf, sizeof(send_buf), recv_buf, sizeof(recv_buf));
    pthread_mutex_unlock(w25qxx_spi_ops->mutex);

    gpiod_line_set_value(cs_gpioline, 1);

    id = (recv_buf[0] << 8) | recv_buf[1];

    return id;
}

/*****************************
 * @brief : 等待 W25QXX 状态寄存器的忙标志位清除
 * @param : 无
 * @return: 无返回值
 *****************************/
static void w25qxx_wait_state_free()
{
    unsigned char send_buf[1] = {W25QXX_READ_STATUS_REGISTER_1};   
    unsigned char recv_buf[1] = {0};

    if(w25qxx_spi_ops == NULL)
        return;

    gpiod_line_set_value(cs_gpioline, 0);

    pthread_mutex_lock(w25qxx_spi_ops->mutex);
    do
    {
        w25qxx_spi_ops->spi_write_then_read(send_buf, sizeof(send_buf), recv_buf, sizeof(recv_buf));
    } while (recv_buf[0] & 0x01);
    pthread_mutex_unlock(w25qxx_spi_ops->mutex);

    gpiod_line_set_value(cs_gpioline, 1);
}

/*****************************
 * @brief : 使能 W25QXX 的写操作
 * @param : 无
 * @return: 无返回值
 *****************************/
static void w25qxx_write_enable()
{
    if(w25qxx_spi_ops == NULL)
        return;

    gpiod_line_set_value(cs_gpioline, 0);

    pthread_mutex_lock(w25qxx_spi_ops->mutex);
    w25qxx_spi_ops->spi_write_byte_data(W25QXX_WRITE_ENABLE);
    pthread_mutex_unlock(w25qxx_spi_ops->mutex);

    gpiod_line_set_value(cs_gpioline, 1);
}

/*****************************
 * @brief : 对指定地址所在的扇区执行 4KB 擦除
 * @param : addr - 扇区的起始地址
 * @return: 无返回值
 *****************************/
void w25qxx_sector_erase(unsigned int addr)
{
    if(w25qxx_spi_ops == NULL)
        return;

    w25qxx_write_enable();

    gpiod_line_set_value(cs_gpioline, 0);

    pthread_mutex_lock(w25qxx_spi_ops->mutex);
    w25qxx_spi_ops->spi_write_byte_data(W25QXX_SECTOR_ERASE_4KB);   // 4K扇区擦除指令
    w25qxx_spi_ops->spi_write_byte_data(addr>>16);                  // 24~16地址
	w25qxx_spi_ops->spi_write_byte_data(addr>>8);                   // 16~8地址
	w25qxx_spi_ops->spi_write_byte_data(addr);                      // 8~0地址
    pthread_mutex_unlock(w25qxx_spi_ops->mutex);

    gpiod_line_set_value(cs_gpioline, 1);

    w25qxx_wait_state_free();                       // 等待操作完成
}

/*****************************
 * @brief : 向指定页写入数据
 * @param : addr - 写入数据的起始地址
 *          write_buff - 待写入的数据缓冲区
 *          len - 待写入数据的长度，最大为 256 字节
 * @return: 无返回值
 * @note  : 向指定页写入数据，若数据超过 256 字节需使用 w25qxx_npage_write() 函数。
 *****************************/
void w25qxx_page_write(unsigned int addr, unsigned char *write_buff, int len)
{
    int i;

    if(w25qxx_spi_ops == NULL)
        return;

    if(len < 1 || len > W25QXX_PAGE_SIZE)
    {
        printf("page write error, data length is invalid!\n");
        return;
    }

    w25qxx_write_enable();

    gpiod_line_set_value(cs_gpioline, 0);

    pthread_mutex_lock(w25qxx_spi_ops->mutex);
    w25qxx_spi_ops->spi_write_byte_data(W25QXX_PAGE_PROGRAM);       // 页编程指令
    w25qxx_spi_ops->spi_write_byte_data(addr>>16);                  // 24~16地址
	w25qxx_spi_ops->spi_write_byte_data(addr>>8);                   // 16~8地址
	w25qxx_spi_ops->spi_write_byte_data(addr);                      // 8~0地址

    for(i = 0; i < len; i++)
	{
		w25qxx_spi_ops->spi_write_byte_data(write_buff[i]);         // 写数据	
	}
	pthread_mutex_unlock(w25qxx_spi_ops->mutex);

    gpiod_line_set_value(cs_gpioline, 1);

    w25qxx_wait_state_free();                       // 等待操作完成
}

/*****************************
 * @brief : 执行多页写入操作，自动处理扇区擦除和分页写入
 * @param : addr - 写入数据的起始地址
 *          write_buff - 存放待写入数据的缓冲区
 *          len - 待写入数据的长度，单位：字节
 * @return: 无返回值
 * @note  : 该函数会根据地址自动处理每次写入的页和扇区的擦除操作。
 *           - 每次写入数据时，最大支持 256 字节（页大小），若数据超出当前页则会继续写入到下一页
 *           - 写入前会判断是否需要擦除当前扇区
 *           - 支持跨页写入，当数据超出当前页时，会继续写入下一个页
 *           - 擦除的最小单位为4KB（一个扇区）
 *           - 写入过程中，如果目标地址跨越多个扇区，会自动进行扇区擦除
 *****************************/
void w25qxx_npage_write(unsigned int addr, unsigned char *write_buff, int len)
{
    int page_size = W25QXX_PAGE_SIZE;               // 每页最大写入256字节
    int offset = 0;                                 // 当前写入数据的偏移量
    int write_len;                                  // 每次写入的数据长度

    int sector = -1;                                // 上次写入的扇区编号
    int current_sector;                             // 当前要写入的扇区编号

    if(w25qxx_spi_ops == NULL)
        return;

    if (len < 1) {
        printf("page write error, data length is invalid!\n");
        return;
    }

    while (len > 0) 
    {
        current_sector = (addr + offset) / W25QXX_SECTOR_SIZE;      // 计算当前数据的目标扇区编号
        if (sector != current_sector)                               // 判断是否跨越扇区，需擦除新的扇区
        {
            sector = current_sector;                
            w25qxx_sector_erase(sector * W25QXX_SECTOR_SIZE);       // 擦除目标扇区
        }
        
        write_len = (len > page_size) ? page_size : len;            // 每次最多写 256 字节，剩余数据长度不超过 256 字节时，调整写入长度

        w25qxx_page_write(addr + offset, write_buff + offset, write_len);

        offset += write_len;                        // 更新偏移量
        len -= write_len;                           // 更新剩余数据长度
    }
}

/*****************************
 * @brief : 从指定地址读取数据
 * @param : addr - 读取数据的起始地址
 *          read_buff - 用于存储读取数据的缓冲区
 *          len - 读取的数据长度
 * @return: 无返回值
 *****************************/
void w25qxx_read_byte_data(unsigned int addr, unsigned char *read_buff, int len)
{
    unsigned char send_buf[4];   

    if(w25qxx_spi_ops == NULL)
        return;

    send_buf[0] = W25QXX_READ_DATA;
    send_buf[1] = addr >> 16;
    send_buf[2] = addr >> 8;
    send_buf[3] = addr;

    gpiod_line_set_value(cs_gpioline, 0);

    pthread_mutex_lock(w25qxx_spi_ops->mutex);
    w25qxx_spi_ops->spi_write_then_read(send_buf, sizeof(send_buf), read_buff, len);
    pthread_mutex_unlock(w25qxx_spi_ops->mutex);

    gpiod_line_set_value(cs_gpioline, 1);
}

/*****************************
 * @brief : 初始化 W25QXX 芯片
 * @param : cs_chip - GPIO 控制器名称
 *          cs_pin - 片选引脚号
 * @return: 成功返回 0，失败返回 -1
 *****************************/
int w25qxx_init(const char *cs_chip, unsigned int cs_pin)
{
    int ret;
    unsigned int device_id;

    ret = w25qxx_cs_pin_init(cs_chip, cs_pin);
    if(ret < 0)
        return -1;

    device_id = w25qxx_read_id();
    switch (device_id)
    {
        case W25QXX_80_ID:
            printf("Find the W25Q80");
            break;
        case W25QXX_16_ID:
            printf("Find the W25Q16");
            break;
        case W25QXX_32_ID:
            printf("Find the W25Q32");
            break;
        case W25QXX_64_ID:
            printf("Find the W25Q64");
            break;
        case W25QXX_128_ID:
            printf("Find the W25Q128");
            break;
        default:
            printf("No storage device found!\n");
            return -1;
    }
    printf(" , id : 0x%x\n", device_id);

    return 0;
}

/*****************************
 * @brief : 注册 SPI 操作函数
 * @param : ops - SPI 操作函数结构体的指针，用于设置 W25QXX 的 SPI 通信接口
 * @return: 无返回值
 *****************************/
void w25qxx_register_spi_operations(spi_operations_t *ops)
{
    w25qxx_spi_ops = ops;
}