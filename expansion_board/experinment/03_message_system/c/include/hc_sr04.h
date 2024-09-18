/*
*
*   file: hc_sr04.h
*   update: 2024-09-12
*
*/

#ifndef _HC_SR04_H
#define _HC_SR04_H

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

int hc_sr04_init(const char *trig_gpiochip, unsigned int trig_gpionum, const char *echo_gpiochip, unsigned int echo_gpionum);
float hc_sr04_get_distance();
void hc_sr04_exit();

#endif