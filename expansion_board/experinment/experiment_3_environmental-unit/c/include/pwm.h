/*
*
*   file: pwm.h
*   update: 2024-08-15
*
*/

#ifndef _PWM_H
#define _PWM_H

#include <gpiod.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <stdint.h>

#define GPIOCHIP_PWM1       "pwmchip1"
#define GPIOCHIP_PWM2       "pwmchip2"

#define PWM1_CHANNEL        "0"
#define PWM2_CHANNEL        "0"

typedef struct pwm {
    char pwmchip[10];
    char channel[10];
}pwm;

int pwm_config(pwm pwmx, const char *attr, const char *val);
int pwm_init(pwm pwmx);
int pwm_exit(pwm pwmx);

#endif