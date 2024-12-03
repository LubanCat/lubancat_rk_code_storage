/*
*
*   file: max30102.h
*   update: 2024-10-25
*
*/

#ifndef _MAX30102_H
#define _MAX30102_H

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

#define FILTER_SIZE 10                          // 滤波器窗口大小

#define MAX30102_ADDRESS                0x57

// 配置寄存器
// STATUS
#define MAX30102_REG_INTR_STATUS1       0x00    // 中断状态寄存器 1
#define MAX30102_REG_INTR_STATUS2       0x01    // 中断状态寄存器 2
#define MAX30102_REG_INTR_ENABLE1       0x02    // 中断使能寄存器 1
#define MAX30102_REG_INTR_ENABLE2       0x03    // 中断使能寄存器 2
// FIFO
#define MAX30102_REG_FIFO_WRITE_PTR     0x04    // FIFO 写指针寄存器
#define MAX30102_REG_OVERFLOW_COUNTER   0x05    // FIFO 溢出计数器寄存器
#define MAX30102_REG_FIFO_READ_PTR      0x06    // FIFO 读指针寄存器
#define MAX30102_REG_FIFO_DATA          0x07    // FIFO 数据寄存器
// CONFIGURATION
#define MAX30102_REG_FIFO_CONF          0x08    // FIFO 配置寄存器
#define MAX30102_REG_MODE_CONF          0x09    // 模式配置寄存器
#define MAX30102_REG_SPO2_CONF          0x0A    // SpO2 配置寄存器
#define MAX30102_REG_LED1_PA            0x0C    // LED1 脉冲幅度配置寄存器
#define MAX30102_REG_LED2_PA            0x0D    // LED2 脉冲幅度配置寄存器
// DIE TEMPERATURE
#define MAX30102_REG_DIE_TEMP_INTEGER   0x1F    // 温度整数部分寄存器
#define MAX30102_REG_DIE_TEMP_FRACTION  0x20    // 温度小数部分寄存器
#define MAX30102_REG_DIE_TEMP_CONFIG    0x21    // 温度配置寄存器
// PART ID
#define MAX30102_REG_REVISION           0xFE    // REV ID Register
#define MAX30102_REG_PARTID             0xFF    // Part ID Register

int max30102_init(uint8_t i2c_bus);
void max30102_read_fifo(uint32_t *data);
void max30102_average_filter(uint32_t new_data_0, uint32_t new_data_1, uint32_t *avg_data_0, uint32_t *avg_data_1);
uint16_t max30102_getHeartRate(float *input_data, uint16_t cache_nums);
float max30102_getSpO2(float *ir_input_data,float *red_input_data,uint16_t cache_nums);
void max30102_exit();

#endif