#define _GNU_SOURCE

#include "parse.h"

#include <unistd.h>
#include <stdint.h>

int readInput( int argc, char *argv[], char *address, uint16_t *port, int *capacity, float *download_speed,
               float *degradation_speed );

int main( int argc, char *argv[] ) {
    char address[16] = "localhost";
    uint16_t port = 5566;
    int capacity;
    float download_speed;
    float degradation_speed;

    readInput( argc, argv, address, &port, &capacity, &download_speed, &degradation_speed );

    return 0;
}

int readInput( int argc, char *argv[], char *address, uint16_t *port, int *capacity, float *download_speed,
               float *degradation_speed ) {
    char c;
    while ( optind < argc ) {
        if (( c = getopt( argc, argv, "c:p:d:" )) != -1 ) {
            switch ( c ) {
                case 'c':
                    *capacity = parseInt( optarg );
                    break;
                case 'p':
                    *download_speed = parseFloat( optarg );
                    break;
                case 'd':
                    *degradation_speed = parseFloat( optarg );
                    break;
            }
        } else {
            *port = parsePort( argv[ optind ] );
            memcpy( address, parseAddress( argv[ optind ] ), strlen( parseAddress( argv[ optind ] )) + 1 );
            optind++;
        }
    }

    return 1;
}