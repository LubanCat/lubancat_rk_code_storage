/*
*
*   file: infrared.h
*   update: 2024-08-14
*
*/

#ifndef _INFRARED_H
#define _INFRARED_H

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h> 

#define GPIOCHIP_DEV            "/dev/gpiochip6"
#define GPIONUM_INFRARED       (8)

struct gpiod_chip *infrared_gpiochip;        
struct gpiod_line *infrared_line;          

int infrared_init(void);
void infrared_exit(void);
int infrared_get_value(void);

#endif