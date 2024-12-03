/*
*
*   file: menu_dht11.c
*   update: 2024-11-02
*
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "oled.h"
#include "menu.h"
#include "key.h"
#include "config.h"

void dht11_info_func(void **params)
{
    int ret;
    uint8_t data[6];
    float temp, humi;
    unsigned char temp_str[50], humi_str[50];
    int dht11_fd;

    if(params == NULL)
        goto _exit;
    menu_t *menu_Host = (menu_t *)(*params);

    /* 1、清屏 */
    oled_clear();

    /* 2、显示菜单项名称 */
    unsigned char *info_name = menu_Host->info[menu_Host->current_page - 1]->name;
    int len = strlen(info_name);
    int start_x = (128 * 0.5) - (len * 8 * 0.5);
    oled_show_string(start_x, 0, info_name);

    /* 3、dht11初始化 */
    cJSON *dht11_dev = config_get_value("dht11", "devname");
    if(dht11_dev == NULL)
        goto _exit;
    dht11_fd = open(dht11_dev->valuestring, O_RDWR);
    if(dht11_fd < 0)
    {
        fprintf(stderr, "can not open file %s, %d\n", dht11_dev->valuestring, dht11_fd);

        oled_show_string(0, 3, "no sensor...");

        while(key_get_value() != KEY2_PRESSED)
            usleep(1000);

        goto _exit;
    }

    /* 4、循环显示温湿度数据 */
    while(key_get_value() != KEY2_PRESSED)
    {
        ret = read(dht11_fd, &data, sizeof(data));	
		if(ret)
        {
            temp = data[2] + data[3] * 0.01;
            humi = data[0] + data[1] * 0.01;
        }
        else
            printf("read data from dth11 err!\n");

        sprintf(temp_str, "temp: %.2f", temp);
        sprintf(humi_str, "humi: %.2f", humi);
        oled_show_string(0, 2, temp_str);
        oled_show_string(0, 4, humi_str);

        usleep(300000);
    }

_exit:

    /* 5、关闭dht11设备描述符 */
    if(dht11_fd >= 0)
        close(dht11_fd);

    oled_clear();
}