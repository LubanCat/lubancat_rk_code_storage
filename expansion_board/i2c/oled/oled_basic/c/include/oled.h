/*
*
*   file: oled.h
*   update: 2024-08-09
*
*/

#ifndef _OLED_H
#define _OLED_H

#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include "i2cbusses.h"
#include <time.h>
#include <stdint.h>
#include <signal.h> 
#include "font.h"

#define SIZE                    (16)
#define OLED_PAGE               8               // OLED页数
#define OLED_COLUMN             128             // OLED列数

#define OLED_I2C_DEV_ADDR       0x3c

void oled_show_char(uint8_t x, uint8_t y, uint8_t achar);
void oled_show_string(uint8_t x, uint8_t y, const uint8_t *string);
void oled_show_chinese(uint8_t x, uint8_t y, uint8_t no);
void oled_clear(void);
void oled_clear_page(int page);
int oled_init(int i2c_bus);

void oled_new_frame();
void oled_show_frame();
void oled_set_pixel(uint8_t x, uint8_t y);
void oled_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void oled_draw_rectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void oled_draw_triangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3);
void oled_draw_circle(uint8_t x, uint8_t y, uint8_t r);
void oled_draw_ellipse(uint8_t x, uint8_t y, uint8_t a, uint8_t b);

#endif