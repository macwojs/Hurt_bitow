#ifndef HURT_BITOW_PRODUCENT_H
#define HURT_BITOW_PRODUCENT_H

#define _GNU_SOURCE

#include "parse.h"
#include "list.h"

#include <fcntl.h>
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
#include <sys/ioctl.h>

#define BLOCK 640
#define SMALL_PACKAGE 3328
#define FULL_PACKAGE 13312
#define CLIENT_LIMIT 1024
#define EPOLL_WAIT_LIMIT (CLIENT_LIMIT + 1)

typedef struct socket_data {
    int fd;
    int data_to_send;
} socket_data;

int readInput( int argc, char *argv[], char *address, uint16_t *port, float *speed );

int createServer( char *address, uint16_t port );

int createEpoll( int soc_fd );

int forkProduce( float rate );

void produce( int pipe, float rate );

int createTimer( int epoll_fd );

void timerReport( int timer_fd, int pipe_fd );

void handleConnection( int soc_fd, int epoll_fd, int pip_fd, int timer_fd );

void connectNewClient( int cl_fd, int epoll_fd, int pipe_fd, list *quote );

void addToEpoll( int epoll_fd, int cl_fd );

void addFromQuote( int epoll_fd, list **quote, int pipe_fd );

void disconnectClient( socket_data *data, int epoll_fd, int pipe_fd );

void sendData( socket_data *data, int epoll_fd, int pipe_fd );

void errorSend( char *msg );


#endif //HURT_BITOW_PRODUCENT_H
