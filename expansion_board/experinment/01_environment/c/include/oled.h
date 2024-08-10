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

#define SIZE    (16)

static uint8_t dev_addr = 0x3c;
static int i2c_bus = 3;
static int file;
static char filename[20];

void oled_show_char(uint8_t x, uint8_t y, uint8_t achar);
void oled_show_string(uint8_t x, uint8_t y, const uint8_t *string);
void oled_show_chinese(uint8_t x, uint8_t y, uint8_t no);
void oled_clear(void);
int oled_init(void);

#endif