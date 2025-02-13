#ifndef _UART_H
#define _UART_H

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

int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop);
int open_port(const char *com);

#endif