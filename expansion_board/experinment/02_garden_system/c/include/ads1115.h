/*
*
*   file: ads1115.h
*   update: 2024-08-14
*
*/

#ifndef _ADS1115_H
#define _ADS1115_H

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
#include "font.h"

#define ADS1115_I2C_BUS                                 (5)
#define ADS1115_I2C_DEV                                 (0x48)

// Address Pointer Register
#define ADS1115_REG_POINTER_CONVERT                     (0x00)
#define ADS1115_REG_POINTER_CONFIG                      (0x01)
#define ADS1115_REG_POINTER_LOWTHRESH                   (0x02)
#define ADS1115_REG_POINTER_HITHRESH                    (0x03)

// Config Register configuration
// 开始单次转换 | 单端输入 0 | 增益 ± 4.096v | 单次触发
#define CONFIG_REG_H                                    (ADS1115_REG_CONFIG_OS_START|\
                                                        ADS1115_REG_CONFIG_MUX_SINGLE_1|\
                                                        ADS1115_REG_CONFIG_PGA_6|\
                                                        ADS1115_REG_CONFIG_MODE_SINGLE)
// 转换速率128 SPS | 传统比较器 | 比较器极性为低电平有效 | 非锁存比较器 | 禁用比较器，并将ALERT/RDY引脚设置为高阻抗
#define CONFIG_REG_L                                    (ADS1115_REG_CONFIG_DR_128|\
                                                        ADS1115_REG_CONFIG_COMP_MODE_TRADITIONAL|\
                                                        ADS1115_REG_CONFIG_COMP_POL_LOW|\
                                                        ADS1115_REG_CONFIG_COMP_LAT_NONLATCH|\
                                                        ADS1115_REG_CONFIG_COMP_QUE_DIS)

// Config Register
// [15] OS 运行状态或单次转换开始
#define ADS1115_REG_CONFIG_OS_START                     (0x1U << 7)     // 开始单次转换（处于掉电状态）
#define ADS1115_REG_CONFIG_OS_NULL                      (0x0U << 7)
// [14:12] MUX[2:0] 输入多路复用器配置
#define ADS1115_REG_CONFIG_MUX_Diff_01                  (0x0U << 4)     // 差分输入0引脚和1引脚 (default)
#define ADS1115_REG_CONFIG_MUX_Diff_03                  (0x1U << 4)     // 差分输入0引脚和3引脚
#define ADS1115_REG_CONFIG_MUX_Diff_13                  (0x2U << 4)     // 差分输入1引脚和3引脚
#define ADS1115_REG_CONFIG_MUX_Diff_23                  (0x3U << 4)     // 差分输入2引脚和3引脚
#define ADS1115_REG_CONFIG_MUX_SINGLE_0                 (0x4U << 4)     // 单端输入 0
#define ADS1115_REG_CONFIG_MUX_SINGLE_1                 (0x5U << 4)     // 单端输入 1
#define ADS1115_REG_CONFIG_MUX_SINGLE_2                 (0x6U << 4)     // 单端输入 2
#define ADS1115_REG_CONFIG_MUX_SINGLE_3                 (0x7U << 4)     // 单端输入 3
// [11:9] PGA[2:0] 可编程的增益放大器（量程的选择）
#define ADS1115_REG_CONFIG_PGA_6                        (0x0U << 1)     // ± 6.1144v
#define ADS1115_REG_CONFIG_PGA_4                        (0x1U << 1)     // ± 4.096v
#define ADS1115_REG_CONFIG_PGA_2                        (0x2U << 1)     // ± 2.048v (default)
#define ADS1115_REG_CONFIG_PGA_1                        (0x3U << 1)     // ± 1.024v
#define ADS1115_REG_CONFIG_PGA_05                       (0x4U << 1)     // ± 0.512v
#define ADS1115_REG_CONFIG_PGA_02                       (0x5U << 1)     // ± 0.256v
// [8] MODE 设备运行方式
#define ADS1115_REG_CONFIG_MODE_SINGLE                  (0x1U << 0)     // 单次 (default)
#define ADS1115_REG_CONFIG_MODE_CONTIN                  (0x0U << 0)     // 连续转换
// [7:5] DR[2:0] 转换速率
#define ADS1115_REG_CONFIG_DR_8                         (0x0U << 5)     // 8 SPS
#define ADS1115_REG_CONFIG_DR_16                        (0x1U << 5)     // 16 SPS 
#define ADS1115_REG_CONFIG_DR_32                        (0x2U << 5)     // 32 SPS
#define ADS1115_REG_CONFIG_DR_64                        (0x3U << 5)     // 64 SPS
#define ADS1115_REG_CONFIG_DR_128                       (0x4U << 5)     // 128 SPS (default)
#define ADS1115_REG_CONFIG_DR_250                       (0x5U << 5)     // 250 SPS
#define ADS1115_REG_CONFIG_DR_475                       (0x6U << 5)     // 475 SPS
#define ADS1115_REG_CONFIG_DR_860                       (0x7U << 5)     // 860 SPS
// [4] COMP_MODE 比较器模式
#define ADS1115_REG_CONFIG_COMP_MODE_TRADITIONAL        (0x0U << 4)     // 传统比较器 (default)
#define ADS1115_REG_CONFIG_COMP_MODE_WINDOW             (0x1U << 4)     // 窗口比较器
// [3] COMP_POL 比较器极性
#define ADS1115_REG_CONFIG_COMP_POL_LOW                 (0x0U << 3)     // 低电平有效 (default)
#define ADS1115_REG_CONFIG_COMP_POL_HIG                 (0x1U << 3)     // 高电平有效
// [2] COMP_LAT 锁存比较器
#define ADS1115_REG_CONFIG_COMP_LAT_NONLATCH            (0x0U << 2)     // 非锁存比较器 (default)
#define ADS1115_REG_CONFIG_COMP_LAT_LATCH               (0x1U << 2)     // 锁存比较器
// [2:0] COMP_QUE[1:0]
#define ADS1115_REG_CONFIG_COMP_QUE_ONE                 (0x0U << 0)     // 1次转换后断言
#define ADS1115_REG_CONFIG_COMP_QUE_TWO                 (0x1U << 0)     // 2次转换后断言
#define ADS1115_REG_CONFIG_COMP_QUE_THR                 (0x2U << 0)     // 4次转换后断言
#define ADS1115_REG_CONFIG_COMP_QUE_DIS                 (0x3U << 0)     // 禁用比较器，并将ALERT/RDY引脚设置为高阻抗

double ads1115_read_vol(void);
int ads1115_init(void);
void ads1115_exit(void);

#endif