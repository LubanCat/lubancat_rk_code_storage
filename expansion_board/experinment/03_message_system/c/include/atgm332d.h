/*
*
*   file: atgm332d.h
*   update: 2024-09-13
*
*/

#ifndef _ATGM332D_H
#define _ATGM332D_H

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>  
#include <stdlib.h>

int atgm332d_init(unsigned char *uart_dev);
int atgm332d_thread_start();
void atgm332d_get_latlon(double *lat, double *lon);
void atgm332d_get_bjtime(unsigned int *year, unsigned int *month, unsigned int *day, unsigned int *hours, unsigned int *min);
void atgm332d_thread_stop();
void atgm332d_exit();

#endif