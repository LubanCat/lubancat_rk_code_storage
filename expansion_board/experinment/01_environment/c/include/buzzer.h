/*
*
*   file: buzzer.h
*   update: 2024-08-09
*
*/

#ifndef _BUZZER_H
#define _BUZZER_H

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h> 

#define GPIONUM_BUZZER  (6)

#define GPIOCHIP_DEV    "/dev/gpiochip6"

struct gpiod_chip *buzzer_gpiochip;    

struct gpiod_line *buzzer_line; 

int buzzer_init(void);
void buzzer_on(void);
void buzzer_off(void);
void buzzer_release(void);

#endif