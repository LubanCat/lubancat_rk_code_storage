#ifndef _HTTP_H
#define _HTTP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

int http_get(const char *youurl, char *buffer);
int http_init();
void http_exit(); 

#endif