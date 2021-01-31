#ifndef HURT_BITOW_PARSE_H
#define HURT_BITOW_PARSE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>


uint16_t parseUInt16( char *buff );

int parseInt( char *buff );

float parseFloat( char *buff );

uint16_t parsePort( char *buff );

char *parseAddress( char *buff );

#endif //HURT_BITOW_PARSE_H
