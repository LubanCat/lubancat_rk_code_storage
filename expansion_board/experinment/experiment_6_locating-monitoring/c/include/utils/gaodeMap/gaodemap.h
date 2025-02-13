#ifndef _GAODEMAP_H
#define _GAPDEMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

typedef struct {
    int status;
    char info[256];
    char infocode[256];
    char locations[256];
} ResponseData;

int gaode_parse_xml(const char *xmlData, ResponseData *response);

#endif