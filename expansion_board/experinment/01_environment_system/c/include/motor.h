/*
*
*   file: motor.h
*   update: 2024-08-09
*
*/

#ifndef _MOTOR_H
#define _MOTOR_H

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
#include <signal.h>

typedef struct pwm {
    char pwmchip[10];
    char channel[10];
}pwm;

int motor_init(const char *stby_gpiochip, const char *ain1_gpiochip, const char *ain2_gpiochip, const char *bin1_gpiochip, const char *bin2_gpiochip, 
               unsigned int stby_gpionum, unsigned int ain1_gpionum, unsigned int ain2_gpionum, unsigned int bin1_gpionum, unsigned int bin2_gpionum,
               const char *pwm_a, const char *pwm_b);
int motor_release(void);
void motor_on(void);
void motor_off(void);
void motor_channelA_direction_set(uint8_t val1, uint8_t val2);
void motor_channelB_direction_set(uint8_t val1, uint8_t val2);
int motor_pwmA_config(const char *attr, const char *val);
int motor_pwmB_config(const char *attr, const char *val);

#endif