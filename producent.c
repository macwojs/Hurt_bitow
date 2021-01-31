#define _GNU_SOURCE

#include "parse.h"

#include <unistd.h>
#include <stdint.h>

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
                    *speed = parseFloat( optarg );
                    break;
            }
        } else {
            *port = parsePort( argv[ optind ] );
            address = parseAddress( argv[ optind ] );
            optind++;
        }
    }

    return 1;
}