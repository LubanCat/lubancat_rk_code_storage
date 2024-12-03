/*
*
*   file: key_buzzer.c
*   update: 2024-10-16
*   usage: 
*       sudo gcc -o key_buzzer key_buzzer.c -lgpiod
*       sudo ./key_buzzer /dev/input/event<X>
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <signal.h> 
#include <gpiod.h>

int fd = -1;

// 定义全局变量，指向GPIO芯片和引脚
struct gpiod_chip *buzzer_gpiochip;        
struct gpiod_line *buzzer_line; 

/*****************************
 * @brief : 初始化蜂鸣器
 * @param : none
 * @return: 0表示成功，-1表示失败
*****************************/
int buzzer_init()
{
    int ret;

    /* get gpio controller */
    buzzer_gpiochip = gpiod_chip_open("/dev/gpiochip6");  
    if(buzzer_gpiochip == NULL)
    {
        printf("gpiod_chip_open error\n");
        return -1;
    }

    /* get gpio line */
    buzzer_line = gpiod_chip_get_line(buzzer_gpiochip, 6);
    if(buzzer_line == NULL)
    {
        printf("gpiod_chip_get_line error : 0\n");
        return -1;
    }

    /* set the line direction to output mode, and the initial level is low */
    ret = gpiod_line_request_output(buzzer_line, "buzzer_line", 0);   
    if(ret < 0)
    {
        printf("gpiod_line_request_output error : buzzer_line\n");
        return -1;
    }

    return 0;
}

/*****************************
 * @brief : 反初始化蜂鸣器
 * @param : none
 * @return: none
*****************************/
void buzzer_exit()
{
    if(buzzer_gpiochip != NULL && buzzer_line != NULL)
    {
        gpiod_line_release(buzzer_line);
        gpiod_chip_close(buzzer_gpiochip);
    }
}

/*****************************
 * @brief : 设置蜂鸣器状态
 * @param : bit - 1表示打开，0表示关闭
 * @return: none
*****************************/
void buzzer_set(int bit)
{
    if(bit)
        gpiod_line_set_value(buzzer_line, 1);
    else
        gpiod_line_set_value(buzzer_line, 0);
}

/*****************************
 * @brief : 处理SIGINT信号的处理函数
 * @param : sig_num - 信号编号
 * @return: none
*****************************/
void sigint_handler(int sig_num) 
{   
    buzzer_set(0);
    buzzer_exit();

    close(fd);
    exit(0);  
}

int main(int argc, char **argv)
{
    struct input_event in_ev = {0};
    
    /* 校验命令行参数 */
    if (2 != argc) {
        fprintf(stderr, "usage: %s <input-dev>\n", argv[0]);
        exit(-1);
    }

    /* 打开输入设备文件 */
    if (0 > (fd = open(argv[1], O_RDONLY))) {
        perror("open error");
        exit(-1);
    }

    /* 注册SIGINT信号的处理函数（Ctrl + C） */
    signal(SIGINT, sigint_handler);

    /* 蜂鸣器初始化 */
    buzzer_init();

    while(1) 
    {
        /* 循环读取输入事件数据 */
        if (sizeof(struct input_event) != read(fd, &in_ev, sizeof(struct input_event))) 
        {
            perror("read error");
            exit(-1);
        }

        /* 检查按键的按下和释放 */
        if(in_ev.code == 11 || in_ev.code == 2 || in_ev.code == 3)
        {
            if(in_ev.value == 1)                    // 按键按下
            {
                printf("buzzer on\n");
                buzzer_set(1);                      // 打开蜂鸣器
            }
            else if(in_ev.value == 0)               // 按键释放
            {
                printf("buzzer off\n");
                buzzer_set(0);                      // 关闭蜂鸣器
            }
        }
    }
}

