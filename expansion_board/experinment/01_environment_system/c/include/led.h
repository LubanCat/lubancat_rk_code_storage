/*
*
*   file: led.h
*   update: 2024-08-09
*
*/

#ifndef _LED_H
#define _LED_H

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h> 

#define GPIONUM_R_LED   (0)
#define GPIONUM_G_LED   (1)
#define GPIONUM_B_LED   (2)

#define GPIOCHIP_DEV    "/dev/gpiochip6"

#define LED_RED         (1)
#define LED_GREEN       (2)
#define LED_BLUE        (3)

int led_init(void);
void led_on(unsigned int led_color);
void led_off(unsigned int led_color);
void led_release(void);

#endif