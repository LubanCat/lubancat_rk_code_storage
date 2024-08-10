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

struct gpiod_chip *led_gpiochip;        

struct gpiod_line *r_led_line;          
struct gpiod_line *g_led_line;       
struct gpiod_line *b_led_line;

int led_init(void);
void led_on(struct gpiod_line *line);
void led_off(struct gpiod_line *line);
void led_release(void);

#endif