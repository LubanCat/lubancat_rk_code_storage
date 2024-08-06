/*
*
*   file: buzzer.c
*   date: 2024-08-06
*   usage: 
*       sudo gcc -o buzzer buzzer.c -lgpiod
*       sudo ./buzzer
*
*/

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define gpionum_buzzer  (6)

int main(int argc, char **argv)
{
    int ret;

    struct gpiod_chip * buzzer_gpiochip;        
    struct gpiod_line * buzzer_line;                 

    /* get gpio controller */
    buzzer_gpiochip = gpiod_chip_open("/dev/gpiochip6");  
    if(buzzer_gpiochip == NULL)
    {
        printf("gpiod_chip_open error\n");
        return -1;
    }

    /* get gpio line */
    buzzer_line = gpiod_chip_get_line(buzzer_gpiochip, gpionum_buzzer);
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

    while(1)
    {   
        /* buzzer off */
        gpiod_line_set_value(buzzer_line, 0);

        /* delay 0.5s */
        usleep(500000);

        /* buzzer on */
        gpiod_line_set_value(buzzer_line, 1);

        usleep(500000);
    }

    return 0;
}