#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <time.h>
#include <signal.h>
#include <sys/timerfd.h>

int readInput( int argc, char *argv[], char *address, uint16_t *port, float *speed );

int main( int argc, char *argv[] ) {
    char address[16] = "localhost";
    uint16_t port = 5566;
    float speed;

    readInput( argc, argv, address, &port, &speed );

    return 0;
}

int readInput( int argc, char *argv[], char *address, uint16_t *port, float *speed ) {
    char c;
    while ( optind < argc ) {
        if (( c = getopt( argc, argv, "p:" )) != -1 ) {
            switch ( c ) {
                case 'p':
                    errno = 0;
                    char *end;
                    float value = strtof( optarg, &end );
                    if ( errno == ERANGE || *end != '\0' ) {
                        perror( "Parsing error(float)\n" );
                        exit( EXIT_FAILURE );
                    }
                    *speed = value;
                    break;
            }
        } else {
            char addr_port[22];
            char delim[] = ":";
            memcpy( addr_port, argv[ optind ], strlen( argv[ optind ] ) + 1 );
            address = strtok( addr_port, delim );

            char *end;
            errno = 0;
            long value = strtol( strtok(NULL, delim ), &end, 10 );
            *port = ( int ) value;
            if ( errno == ERANGE || *end != '\0' ) {
                perror( "Parsing error(int)\n" );
                exit( EXIT_FAILURE );
            }
            optind++;
        }
    }

    return 1;
}