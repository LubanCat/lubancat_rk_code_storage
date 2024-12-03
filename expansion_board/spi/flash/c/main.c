/*
*
*   file: main.c
*   update: 2024-11-11
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

#include "w25qxx.h"
#include "key.h"

#define KEY_EVENT		"/dev/input/event9"
#define SPI_BUS			"/dev/spidev3.0"

#define CS_GPIO_CHIP	"/dev/gpiochip6"
#define CS_GPIO_NUM		(11)

#define FLASH_ADDR		(0x000000)

int main(int argc, char **argv)
{
	int ret;
	int key_value;

	unsigned char rand_num[1] = {0};
	unsigned char read_buff[1] = {0};

	/* 按键初始化 */
	ret = key_init(KEY_EVENT);
    if(ret == -1)
    {
        printf("key init error!\n");
        return -1;
    }

	/* w25qxx初始化 */
    ret = w25qxx_init(SPI_BUS, CS_GPIO_CHIP, CS_GPIO_NUM);
	if(ret < 0)
	{
		printf("w25qxx init error!\n");
		return -1;
	}

	/* 程序启动时先读取一次flash */
	w25qxx_read_byte_data(FLASH_ADDR, read_buff, sizeof(read_buff));
	printf("read one byte : 0x%x\n", read_buff[0]);

	while (1)
	{
		key_value = key_get_value();            

		if(key_value == KEY1_PRESSED)			// key1生成随机数，并写入flash
		{
			rand_num[0] = rand() % 256;
			w25qxx_npage_write(FLASH_ADDR, rand_num, sizeof(rand_num));
			printf("write one byte : 0x%x\n", rand_num[0]);
		}
		else if(key_value == KEY2_PRESSED)		// key2读取数据
		{
			w25qxx_read_byte_data(FLASH_ADDR, read_buff, sizeof(read_buff));
			printf("read one byte : 0x%x\n", read_buff[0]);
		}

		sleep(0.5);
	}

	return 0;
}