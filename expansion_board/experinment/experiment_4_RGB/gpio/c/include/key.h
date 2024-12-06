/*
*
*   file: key.h
*   update: 2024-09-12
*
*/

#ifndef _KEY_H
#define _KEY_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <pthread.h>  
#include <signal.h> 

#define KEY_BUFFER_SIZE     10          // 缓冲区大小

#define KEY_NORMAL          (0)
#define KEY1_PRESSED        (11)
#define KEY2_PRESSED        (12)
#define KEY3_PRESSED        (13)

int key_init(const char *key_event);
int key_get_value();
void key_exit();

#endif