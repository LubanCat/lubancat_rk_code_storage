/*
*
*   file: ec11.h
*   update: 2024-09-12
*
*/

#ifndef _EC11_H
#define _EC11_H

#include <stdio.h>  
#include <stdlib.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <string.h>  
#include <linux/input.h>  
#include <pthread.h>  
#include <errno.h>  

#define EC11_NORMAL                     (0)         // 按键无动作
#define EC11_TURN_RIGHT                 (1)         // 按键顺时针旋转
#define EC11_TURN_LEFT                  (2)         // 按键逆时针旋转
#define EC11_TURN_RIGHT_PRESS           (3)         // 按键顺时针旋转且按下
#define EC11_TURN_LEFT_PRESS            (4)         // 按键逆时针旋转且按下
#define EC11_PRESS                      (5)         // 按键按下

int ec11_init(const unsigned char *ec11_sw_event, const unsigned char *ec11_A_event, const unsigned char *ec11_B_event);
int ec11_thread_start(void);
int ec11_get_value(void);
void ec11_thread_stop(void);
void ec11_exit(void);

#endif