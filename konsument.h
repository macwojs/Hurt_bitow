#ifndef HURT_BITOW_KONSUMENT_H
#define HURT_BITOW_KONSUMENT_H

#define _GNU_SOURCE

#include "parse.h"

#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/timerfd.h>

#define STORAGE 30720 //30KiB


int readInput( int argc, char *argv[], char *address, uint16_t *port, int *capacity, float *download_speed,
               float *degradation_speed );

int connectToServer( char *address, uint16_t port );

void getData( int soc_fd, int capacity, float download_speed, float degradation_speed );

#endif //HURT_BITOW_KONSUMENT_H
