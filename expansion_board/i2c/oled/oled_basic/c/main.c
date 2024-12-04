/*
*
*   file: main.c
*   update: 2024-10-22
*   usage: 
*       make
*       sudo ./main
*
*/

#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <signal.h> 

#include "oled.h"

int i2c_bus = 3;

void sigint_handler(int sig_num) 
{    
    oled_clear();
    exit(0);  
}

int main(int argc, char **argv)
{
    int ret;

    signal(SIGINT, sigint_handler);

    // oled初始化
    ret = oled_init(i2c_bus);
    if(ret == -1)
    {
        printf("oled init err!\n");
        return -1;
    }
    
    oled_new_frame();
    oled_draw_rectangle(5, 5, 20, 20);
    oled_draw_circle(45, 15, 10);
    oled_draw_triangle(75, 5, 65, 25, 85, 25);
    oled_show_frame();

    oled_show_chinese(0, 6, 0);
    oled_show_chinese(16, 6, 1);
    oled_show_string(32, 6, "Lubancat");

    while(1)
    {
        sleep(1);
    }

    return 0;
}