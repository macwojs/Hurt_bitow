#include "konsument.h"

int main( int argc, char *argv[] ) {
    char address[16] = "localhost";
    uint16_t port;
    int capacity;
    float download_speed;
    float degradation_speed;

    readInput( argc, argv, address, &port, &capacity, &download_speed, &degradation_speed );

    int soc_fd = connectToServer( address, port );

    getData( soc_fd, capacity, download_speed, degradation_speed );

    return 0;
}


void getData( int soc_fd, int capacity, float download_speed, float degradation_speed ) {
    char send_buffer = 'r';
    long long storage = capacity * STORAGE;
    long long actual_storage = 0;
    char buffer[13312] = {};
    while ( 1 ) {
        //Send request for data from server
        if ( send( soc_fd, &send_buffer, sizeof( send_buffer ), NULL) == -1 ) {
            perror( "Cant request for data" );
            exit( EXIT_FAILURE );
        }

        //Get data from server
        int recv_result = recv( soc_fd, buffer, sizeof( buffer ), NULL);
        if ( recv_result == -1 && (errno == EAGAIN || errno == EWOULDBLOCK )) {
            errno = 0;
        } else if ( recv_result == -1 ) {
            perror( "Cant read data from server" );
            exit( EXIT_FAILURE );
        }

        //Processing and degradation
        actual_storage += recv_result;

        struct timespec ts = {};
        size_t sleep_ns = 13312 / ( download_speed * 4435 ) * 1e9;
        ts.tv_sec = sleep_ns / 1e9;
        ts.tv_nsec = sleep_ns % ( size_t ) ( 1e9 );



        nanosleep( &ts, NULL);


        //TODO: Wait for process data
        //TODO: Degradate data


    }
}

int connectToServer( char *address, uint16_t port ) {
    int client_socket = socket( AF_INET, SOCK_STREAM, 0 );
    if ( client_socket == -1 ) {
        perror( "Can't create client socket\n" );
        exit( EXIT_FAILURE );
    }

    struct sockaddr_in addr;
    memset( &addr, 0, sizeof( struct sockaddr_in ));
    addr.sin_port = htons( port );
    addr.sin_family = AF_INET;
    if ( inet_aton( address, &addr.sin_addr ) == -1 ) {
        perror( "Error during parsing address\n" );
        exit( EXIT_FAILURE );
    }

    if ( connect( client_socket, &addr, sizeof( addr )) == -1 ) {
        perror( "Error during connecting to server\n" );
        exit( EXIT_FAILURE );
    }

    return client_socket;
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
            if ( strlen( argv[ optind ] ) <= 5 ) {
                *port = parseUInt16( argv[ optind ] );
                optind++;
            } else {
                *port = parsePort( argv[ optind ] );
                memcpy( address, parseAddress( argv[ optind ] ), strlen( parseAddress( argv[ optind ] )) + 1 );
                optind++;
            }
        }
    }

    return 1;
}