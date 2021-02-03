#ifndef HURT_BITOW_KONSUMENT_H
#define HURT_BITOW_KONSUMENT_H

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309

#define UNUSED(x) (void)(x)

#include "parse.h"

#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/timerfd.h>

#define STORAGE 30720 //30KiB
#define SMALL_PACKAGE 3328
#define FULL_PACKAGE 13312


int readInput( int argc, char *argv[], char *address, uint16_t *port, int *capacity, float *download_speed,
               float *degradation_speed );

int connectToServer( char *address, uint16_t port );

int getData( int capacity, float download_speed, float degradation_speed,  char *address, uint16_t port );

void on_exit_report( int status, void *dn );

void errorSend( char *msg );

typedef struct report {
    struct timespec *a;
    struct timespec *b;
} report;

#endif //HURT_BITOW_KONSUMENT_H
