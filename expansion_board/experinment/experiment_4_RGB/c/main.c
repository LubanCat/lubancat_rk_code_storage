/*
*
*   file: main.c
*   update: 2024-11-25
*   usage: 
*       make
*       sudo ./main
*
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <signal.h> 
#include <pthread.h>

#include "config.h"
#include "key.h"
#include "w25qxx.h"

// 配置灯光和颜色相关的常量
#define COLOR_TABLE_SIZE                20                          // 支持的颜色总数
#define CURRENT_COLORS_SIZE             4                           // 当前灯光颜色数量

// W25Qxx SPI Flash RGB颜色数据存储地址
#define W25Qxx_ADDR_RGB_COLOR           (0x000000)

// WS2812灯光控制结构体
struct ws2812_mes 
{
    unsigned int gpiochip;              // data引脚的gpiochip
    unsigned int gpionum;               // data引脚的gpionum
    unsigned int lednum;                // 要控制灯带的第几个LED，序号从1开始
    unsigned char color[3];             // color[0]:color[1]:color[2]   R:G:B 
};

// 全局变量定义
int ws2812_fd;                          // ws2812文件描述符
pthread_t ws2812_obj;                   // ws2812线程对象
int ws2812_thread_stop = 0;             // ws2812线程停止标志
struct ws2812_mes ws2812;               // ws2812灯光控制信息

unsigned char indices[CURRENT_COLORS_SIZE+1] = {0, 3, 2, 1, 0};     // 当前灯光颜色索引及写入标志。最后一个值存放flash写入标记，1已写入，0未写入
pthread_mutex_t current_colors_mutex = PTHREAD_MUTEX_INITIALIZER;   // 颜色更新互斥锁
unsigned char current_colors[CURRENT_COLORS_SIZE][3] = {0};         // 当前灯光颜色数组
unsigned char color_table[][3] = {
    {255, 0, 0},    // 红
    {0, 255, 0},    // 绿
    {0, 0, 255},    // 蓝
    {255, 255, 0},  // 黄
    {0, 255, 255},  // 青
    {255, 0, 255},  // 洋红
    {128, 128, 128},// 灰
    {255, 165, 0},  // 橙
    {128, 0, 128},  // 紫
    {0, 128, 128},  // 蓝绿
    {192, 192, 192},// 浅灰
    {128, 128, 0},  // 橄榄绿
    {255, 192, 203},// 粉红
    {75, 0, 130},   // 靛蓝
    {173, 216, 230},// 浅蓝
    {245, 222, 179},// 小麦色
    {60, 179, 113}, // 中海绿
    {220, 20, 60},  // 猩红
    {240, 230, 140},// 卡其色
    {139, 69, 19}   // 棕色
};

/*****************************
 * @brief : 信号处理函数，用于清理线程和关闭设备
 * @param : sig_num - 捕获的信号编号
 * @return: 无返回值
 *****************************/
void sigint_handler(int sig_num) 
{    
    /* 退出ws2812线程 */
    ws2812_thread_stop = 1;
    pthread_mutex_destroy(&current_colors_mutex);
    pthread_join(ws2812_obj, NULL);

    /* 关闭ws2812 */
    close(ws2812_fd);

    exit(0);  
}

/*****************************
 * @brief : 更新当前使用的颜色数组
 * @param : 无参数
 * @return: 无返回值
 *****************************/
void update_current_colors() 
{
    pthread_mutex_lock(&current_colors_mutex);
    for (int i = 0; i < CURRENT_COLORS_SIZE; i++) 
        for (int j = 0; j < 3; j++) 
            current_colors[i][j] = color_table[indices[i]][j];
    pthread_mutex_unlock(&current_colors_mutex);
}

/*****************************
 * @brief : 生成一组唯一的随机颜色索引
 * @param : 无参数
 * @return: 无返回值
 *****************************/
void generate_unique_random_indices() 
{
    int temp, is_unique;

    for (int i = 0; i < CURRENT_COLORS_SIZE; i++) 
    {
        do {
            temp = rand() % COLOR_TABLE_SIZE;   // 随机生成索引
            is_unique = 1;                      // 假设数字是唯一的

            // 检查是否与之前的数字重复
            for (int j = 0; j < i; j++) 
            {
                if (indices[j] == temp) 
                {
                    is_unique = 0;              // 如果重复，设置为非唯一
                    break;
                }
            }
        } while (!is_unique);                   // 如果不唯一，则重新生成

        indices[i] = temp;                      // 存储唯一数字
    }
}

/*****************************
 * @brief : 将当前颜色索引保存到FLASH存储
 * @param : 无参数
 * @return: 无返回值
 *****************************/
void save_color_indices_to_flash()
{
    indices[CURRENT_COLORS_SIZE] = 1;           // 写入标志
    w25qxx_npage_write(W25Qxx_ADDR_RGB_COLOR, indices, sizeof(indices));
    printf("保存颜色数据到Flash成功!\n");
}

/*****************************
 * @brief : 从FLASH存储中加载颜色索引
 * @param : 无参数
 * @return: 无返回值
 *****************************/
void update_color_indices_from_flash()
{
    unsigned char read_buff[CURRENT_COLORS_SIZE+1] = {0};
    w25qxx_read_byte_data(W25Qxx_ADDR_RGB_COLOR, read_buff, sizeof(read_buff));

    if(read_buff[CURRENT_COLORS_SIZE] == 1)
    {   
        int is_valid = 1;
        for(int i = 0; i < CURRENT_COLORS_SIZE; i++)
            if(read_buff[i] >= COLOR_TABLE_SIZE)
                is_valid = 0;
                
        if(is_valid)
        {
            memcpy(indices, read_buff, sizeof(indices) - 1);
            printf("加载颜色数据成功!\n");
        }
        else
        {
            printf("颜色数据校验失败, 使用默认颜色!\n");
        }
    }    
    else
    {
        printf("未检测到有效的颜色标志, 使用默认颜色!\n");
    }
}

/*****************************
 * @brief : 计算两数之间的插值，用于颜色过渡计算
 * @param : start - 起始值
 * @param : end   - 结束值
 * @param : ratio - 插值比例，范围为0.0到1.0
 * @return: 插值结果，类型为unsigned char
 *****************************/
unsigned char interpolate(unsigned char start, unsigned char end, float ratio) 
{
    return (unsigned char)(start + (end - start) * ratio);
}

/*****************************
 * @brief : WS2812灯光线程，动态更新灯光颜色
 * @param : arg - 线程参数，当前为NULL
 * @return: 无返回值
 *****************************/
void *ws2812_thread()
{
    int color_count = sizeof(current_colors) / sizeof(current_colors[0]);
    int currc = 0, nextc = 1;  

    while(ws2812_thread_stop != 1)
    {
        // 从暗到亮
        for (int i = 0; i <= 100; i++) 
        {
            float brightness = i / 100.0;                           // 当前亮度比例
            float color_ratio = (i > 50) ? (i - 50) / 50.0 : 0;     // 半程后开始颜色过渡

            pthread_mutex_lock(&current_colors_mutex);
            unsigned char r = interpolate(current_colors[currc][0], current_colors[nextc][0], color_ratio);
            unsigned char g = interpolate(current_colors[currc][1], current_colors[nextc][1], color_ratio);
            unsigned char b = interpolate(current_colors[currc][2], current_colors[nextc][2], color_ratio);
            pthread_mutex_unlock(&current_colors_mutex);

            ws2812.color[0] = (unsigned char)(r * brightness);
            ws2812.color[1] = (unsigned char)(g * brightness);
            ws2812.color[2] = (unsigned char)(b * brightness);

            write(ws2812_fd, &ws2812, sizeof(ws2812));
            usleep(20000);                                          // 调整呼吸速度
        }

        // 从亮到暗
        for (int i = 100; i >= 0; i--) 
        {
            float brightness = i / 100.0;                           // 当前亮度比例
            float color_ratio = (i < 50) ? (50 - i) / 50.0 : 0;     // 半程后开始颜色过渡

            pthread_mutex_lock(&current_colors_mutex);
            unsigned char r = interpolate(current_colors[nextc][0], current_colors[currc][0], color_ratio);
            unsigned char g = interpolate(current_colors[nextc][1], current_colors[currc][1], color_ratio);
            unsigned char b = interpolate(current_colors[nextc][2], current_colors[currc][2], color_ratio);
            pthread_mutex_unlock(&current_colors_mutex);

            ws2812.color[0] = (unsigned char)(r * brightness);
            ws2812.color[1] = (unsigned char)(g * brightness);
            ws2812.color[2] = (unsigned char)(b * brightness);

            write(ws2812_fd, &ws2812, sizeof(ws2812));
            usleep(20000);                                          // 调整呼吸速度
        }

        // 更新当前和下一个颜色索引
        currc = (currc + 1) % color_count;
        nextc = (currc + 1) % color_count;      
    }                  

    printf("%s has been exit!\n", __FUNCTION__);
    pthread_exit(NULL);
}

int main(int argc, char **argv) 
{
    int ret;
    int key_value = 0;
     
    ws2812.gpiochip = 3;
    ws2812.gpionum = 19;
    ws2812.lednum = 1;

    /* 设置随机数种子 */
    srand(time(NULL));

    /* 注册信号函数 */
    signal(SIGINT, sigint_handler);

    /* 初始化配置文件 */
    ret = config_init(CONFIG_FILE_NAME);
    if(ret < 0)
    {
        printf("config init error!\n");
        return -1;
    }

    /* 板载按键初始化 */
    cJSON *key_event = config_get_value("key", "event");
    if(key_event == NULL)
        return -1;

    ret = key_init(key_event->valuestring);
    if(ret == -1)
    {
        fprintf(stderr, "key init error!\n");
        return -1;
    }

    /* 打开ws2812字符设备 */
    cJSON *ws2812_event = config_get_value("ws2812", "devname");
    if(ws2812_event == NULL)
        return -1;

    ws2812_fd = open(ws2812_event->valuestring, O_RDWR);
    if (ws2812_fd == -1) 
    {
        perror("Failed to open /dev/ws2812!\n");
        return -1;
    }

    /* w25qxx初始化 */
    char w25qxxspi[20], w25qxxcschip[20];
    cJSON *w25qxx_spi = config_get_value("w25qxx", "bus");
    cJSON *cs_chip = config_get_value("w25qxx", "cs_chip");
    cJSON *cs_pin = config_get_value("w25qxx", "cs_pin");
    if(w25qxx_spi == NULL || cs_chip == NULL || cs_pin == NULL)
        return -1;

    sprintf(w25qxxspi, "/dev/spidev%d.0", w25qxx_spi->valueint);
    sprintf(w25qxxcschip, "/dev/gpiochip%s", cs_chip->valuestring);
    ret = w25qxx_init(w25qxxspi, w25qxxcschip, cs_pin->valueint);
	if(ret < 0)
	{
		printf("w25qxx init error!\n");
		return -1;
	}

    /* 检测flash是否有已保存好的氛围灯颜色数据 */
    update_color_indices_from_flash();
    update_current_colors();

    /* 创建ws2812呼吸灯线程 */
    ret = pthread_create(&ws2812_obj, NULL, ws2812_thread, NULL);  
    if(ret < 0)
    {
        fprintf(stderr, "ws2812_thread create error!\n");
        return -1;
    }

    while (1) 
    {
        key_value = key_get_value();            
        switch (key_value)
        {
            case KEY1_PRESSED:
                generate_unique_random_indices();
                printf("已在本地生成新的随机颜色数据!\n");
                update_current_colors();
                save_color_indices_to_flash();
                break;

            case KEY2_PRESSED:
                w25qxx_sector_erase(W25Qxx_ADDR_RGB_COLOR);
                printf("已擦除FLASH存储中的颜色数据!\n");
                break;

            default:
                break;
        }

        sleep(1);
    }

    
    return 0;
}
