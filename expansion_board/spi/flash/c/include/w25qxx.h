#ifndef _W25QXX_H
#define _W25QXX_H

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

// 设备ID
#define W25QXX_80_ID						0xEF13
#define W25QXX_16_ID						0xEF14
#define W25QXX_32_ID						0xEF15
#define W25QXX_64_ID						0xEF16
#define W25QXX_128_ID						0xEF17	

#define W25QXX_PAGE_SIZE					(256)
#define W25QXX_SECTOR_SIZE					(4096)

// 常用操作命令
#define W25QXX_WRITE_ENABLE                	0x06    // 使能写操作
#define W25QXX_VOLATILE_SR_WRITE_ENABLE    	0x50    // 使能可变状态寄存器写
#define W25QXX_WRITE_DISABLE               	0x04    // 禁止写操作

// 低功耗操作
#define W25QXX_RELEASE_POWER_DOWN_ID       	0xAB    // 退出掉电模式，读设备ID

// 读设备 ID 命令
#define W25QXX_MANUFACTURER_DEVICE_ID      	0x90    // 制造商/设备ID
#define W25QXX_JEDEC_ID                    	0x9F    // JEDEC ID
#define W25QXX_READ_UNIQUE_ID              	0x4B    // 读取唯一ID

// 读数据操作
#define W25QXX_READ_DATA                   	0x03    // 读取数据
#define W25QXX_FAST_READ                   	0x0B    // 快速读取数据

// 写数据操作
#define W25QXX_PAGE_PROGRAM                	0x02    // 页编程

// 擦除操作
#define W25QXX_SECTOR_ERASE_4KB            	0x20    // 4KB 扇区擦除
#define W25QXX_BLOCK_ERASE_32KB            	0x52    // 32KB 块擦除
#define W25QXX_BLOCK_ERASE_64KB            	0xD8    // 64KB 块擦除
#define W25QXX_CHIP_ERASE                  	0xC7    // 整个芯片擦除
#define W25QXX_CHIP_ERASE_ALT              	0x60    // 整个芯片擦除（备用命令）

// 状态寄存器操作
#define W25QXX_READ_STATUS_REGISTER_1      	0x05    // 读取状态寄存器-1
#define W25QXX_WRITE_STATUS_REGISTER_1     	0x01    // 写入状态寄存器-1
#define W25QXX_READ_STATUS_REGISTER_2      	0x35    // 读取状态寄存器-2
#define W25QXX_WRITE_STATUS_REGISTER_2     	0x31    // 写入状态寄存器-2
#define W25QXX_READ_STATUS_REGISTER_3      	0x15    // 读取状态寄存器-3
#define W25QXX_WRITE_STATUS_REGISTER_3     	0x11    // 写入状态寄存器-3

// SFDP 寄存器操作
#define W25QXX_READ_SFDP_REGISTER          	0x5A    // 读取 SFDP 寄存器

// 安全寄存器操作
#define W25QXX_ERASE_SECURITY_REGISTER     	0x44    // 擦除安全寄存器
#define W25QXX_PROGRAM_SECURITY_REGISTER   	0x42    // 编程安全寄存器
#define W25QXX_READ_SECURITY_REGISTER      	0x48    // 读取安全寄存器

// 块锁定/解锁操作
#define W25QXX_GLOBAL_BLOCK_LOCK           	0x7E    // 全局块锁定
#define W25QXX_GLOBAL_BLOCK_UNLOCK         	0x98    // 全局块解锁
#define W25QXX_READ_BLOCK_LOCK             	0x3D    // 读取块锁定状态
#define W25QXX_INDIVIDUAL_BLOCK_LOCK       	0x36    // 单个块锁定
#define W25QXX_INDIVIDUAL_BLOCK_UNLOCK     	0x39    // 单个块解锁

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

void w25qxx_sector_erase(unsigned int addr);
void w25qxx_page_write(unsigned int addr, unsigned char *buff, int len);
void w25qxx_npage_write(unsigned int addr, unsigned char *write_buff, int len);
void w25qxx_read_byte_data(unsigned int addr, unsigned char *buff, int len);
int w25qxx_init(const char *spi_dev, const char *cs_chip, unsigned int cs_pin);

#endif