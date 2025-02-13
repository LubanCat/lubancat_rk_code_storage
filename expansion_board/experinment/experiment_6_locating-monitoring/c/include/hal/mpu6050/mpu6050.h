/*
*
*   file: mpu6050.h
*   update: 2024-09-13
*
*/

#ifndef _MPU6050_H
#define _MPU6050_H

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

int mpu6050_init(uint8_t i2c_bus, uint8_t dev_addr);
void mpu6050_read_gyro(short int *buf);
void mpu6050_read_accel(short int *buf);
void mpu6050_exit();

#endif