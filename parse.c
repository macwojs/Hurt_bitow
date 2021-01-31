#include "parse.h"

uint16_t parseUInt16( char *buff ) {
    char *end;
    errno = 0;
    long value = strtol( buff, &end, 10 );

    uint16_t ret = ( uint16_t ) value;

    if ( errno == ERANGE || ret != value || *end != '\0' ) {
        perror( "Parsing error(uint16_t)\n" );
        exit( EXIT_FAILURE );
    }

    return ret;
}

int parseInt( char *buff ) {
    char *end;
    errno = 0;
    long value = strtol( buff, &end, 10 );

    int ret = ( int ) value;

    if ( errno == ERANGE || ret != value || *end != '\0' ) {
        perror( "Parsing error(int)\n" );
        exit( EXIT_FAILURE );
    }

    return ret;
}

float parseFloat( char *buff ) {
    char *end;
    errno = 0;
    float value = strtof( buff, &end );

    if ( errno == ERANGE || *end != '\0' ) {
        perror( "Parsing error(float)\n" );
        exit( EXIT_FAILURE );
    }

    return value;
}

uint16_t parsePort( char *buff ) {
    char delim[] = ":";
    strtok( buff, delim );

    return parseUInt16( strtok(NULL, delim ));
}

char *parseAddress( char *buff ) {
    char delim[] = ":";
    return strtok( buff, delim );
}