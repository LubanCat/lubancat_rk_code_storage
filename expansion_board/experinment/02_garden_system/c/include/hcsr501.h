/*
*
*   file: hcsr501.h
*   update: 2024-08-14
*
*/

#ifndef _HCSR501_H
#define _HCSR501_H

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h> 

struct gpiod_chip *hcsr501_gpiochip;        
struct gpiod_line *hcsr501_line;          

int hcsr501_init(const char *gpiochip, unsigned int gpionum);
void hcsr501_exit(void);
int hcsr501_get_value(void);

#endif