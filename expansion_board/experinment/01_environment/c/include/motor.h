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

/* gpiochip */
#define GPIOCHIP_MOTORCTRL_STBY     "/dev/gpiochip1"
#define GPIOCHIP_MOTORCTRL_AIN1     "/dev/gpiochip4"
#define GPIOCHIP_MOTORCTRL_AIN2     "/dev/gpiochip3"
#define GPIOCHIP_MOTORCTRL_BIN1     "/dev/gpiochip1"
#define GPIOCHIP_MOTORCTRL_BIN2     "/dev/gpiochip1"

#define GPIOCHIP_PWMA               "pwmchip1"
#define GPIOCHIP_PWMB               "pwmchip2"

/* gpionum  
 *
 * A-D : 0-3
 * number = group * 8 + x
 * e.g. : B0 = 1 * 8 + 0 = 8
 *	      C4 = 2 * 8 + 4 = 20 
 */
#define GPIONUM_MOTORCTRL_STBY      (8)
#define GPIONUM_MOTORCTRL_AIN1      (20)
#define GPIONUM_MOTORCTRL_AIN2      (8)
#define GPIONUM_MOTORCTRL_BIN1      (9)
#define GPIONUM_MOTORCTRL_BIN2      (10)

struct gpiod_chip *STBY_gpiochip;
struct gpiod_chip *AIN1_gpiochip;   
struct gpiod_chip *AIN2_gpiochip;
struct gpiod_chip *BIN1_gpiochip;
struct gpiod_chip *BIN2_gpiochip;     

struct gpiod_line *STBY_line;
struct gpiod_line *AIN1_line;   
struct gpiod_line *AIN2_line;
struct gpiod_line *BIN1_line;
struct gpiod_line *BIN2_line;

typedef struct pwm {
    char pwmchip[10];
    char channel[10];
}pwm;
pwm pwma, pwmb;

int motor_init(uint8_t AIN1, uint8_t AIN2, uint8_t BIN1, uint8_t BIN2);
int motor_release(void);
void motor_on(void);
void motor_off(void);
void motor_channelA_direction_set(uint8_t val1, uint8_t val2);
void motor_channelB_direction_set(uint8_t val1, uint8_t val2);
int motor_pwmA_config(const char *attr, const char *val);
int motor_pwmB_config(const char *attr, const char *val);

#endif