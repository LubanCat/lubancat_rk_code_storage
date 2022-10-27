#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
int main(int argc, char **argv)
{
    int i;
    int ret;
    struct gpiod_chip * chip;      //GPIO控制器句柄
    struct gpiod_line * line;      //GPIO引脚句柄
    /*
    获取GPIO控制器
    */
    chip = gpiod_chip_open("/dev/gpiochip1");
    if(chip == NULL)
    {
        printf("gpiod_chip_open error\n");
        return -1;
    }

    /*获取GPIO引脚*/
    line = gpiod_chip_get_line(chip, 10);
    if(line == NULL)
    {
        printf("gpiod_chip_get_line error\n");
        goto release_line;
    }

    /*设置GPIO为输出模式*/
    ret = gpiod_line_request_output(line,"led",0);
    if(ret < 0)
    {
        printf("gpiod_line_request_output error\n");
        goto release_chip;
    }

    for(i = 0;i<10;i++)
    {
        gpiod_line_set_value(line,1);
        usleep(500000);  //延时0.5s
        gpiod_line_set_value(line,0);
        usleep(500000);
    }
    
release_line:
    /*释放GPIO引脚*/
    gpiod_line_release(line);
release_chip:
    /*释放GPIO控制器*/
    gpiod_chip_close(chip);
    return 0;
}