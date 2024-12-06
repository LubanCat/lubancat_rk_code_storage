/*
*
*   file: config.h
*   update: 2024-09-20
*
*/

#ifndef _CONFIG_H
#define _CONFIG_H

#include "cJSON.h"
#include <stdio.h>  
#include <stdlib.h>
#include <string.h>

#define CONFIG_FILE_NAME    "../configuration.json"

int config_init(const char *filename);
cJSON *config_get_value(const char *component, const char *key);
void config_free(void);

#endif