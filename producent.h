#ifndef HURT_BITOW_PRODUCENT_H
#define HURT_BITOW_PRODUCENT_H

#define _GNU_SOURCE

#include "parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <time.h>
#include <signal.h>
#include <sys/timerfd.h>

#define CLIENT_LIMIT 1024
#define EPOLL_WAIT_LIMIT (CLIENT_LIMIT + 1)

int readInput( int argc, char *argv[], char *address, uint16_t *port, float *speed );

int createServer( char *address, uint16_t port );

int createEpoll( int soc_fd );

int forkProduce();

void handleConnection(int soc_fd, int epoll_fd, int pip_fd);

#endif //HURT_BITOW_PRODUCENT_H
