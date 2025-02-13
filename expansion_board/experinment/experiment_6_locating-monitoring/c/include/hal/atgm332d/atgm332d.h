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

int atgm332d_init(char *uart_dev);
int atgm332d_thread_start();
void atgm332d_get_latlon_dd(double *lat, double *lon);
void atgm332d_exit();

#endif