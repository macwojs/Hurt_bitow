#ifndef HURT_BITOW_KONSUMENT_H
#define HURT_BITOW_KONSUMENT_H

#define _GNU_SOURCE

#include "parse.h"

#include <unistd.h>
#include <stdint.h>


int readInput( int argc, char *argv[], char *address, uint16_t *port, int *capacity, float *download_speed,
               float *degradation_speed );


#endif //HURT_BITOW_KONSUMENT_H
