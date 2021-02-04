#include "konsument.h"

int main( int argc, char *argv[] ) {
    char address[16] = "localhost";
    uint16_t port;
    int capacity;
    float download_speed;
    float degradation_speed;
    struct sockaddr_in local_address;

    readInput( argc, argv, address, &port, &capacity, &download_speed, &degradation_speed );

    getData( capacity, download_speed, degradation_speed, address, port, &local_address );

    sendReport( &local_address );

    return 0;
}

void sendReport( struct sockaddr_in *local_address ) {
    struct timespec finish_time;
    if ( clock_gettime( CLOCK_REALTIME, &finish_time ) == -1 )
        errorSend( "Error during get finish time" );

    fprintf( stderr, "\nFinish time %li sec, %li nsec\t Address: %s:%d\t PID: %d\n",
             finish_time.tv_sec, finish_time.tv_nsec, inet_ntoa( local_address->sin_addr ),
             ntohs( local_address->sin_port ), getpid());
}

void
generateReport( struct timespec connect_time, struct timespec first_package_time, struct timespec last_package_time ) {
    report *report_data = ( report * ) calloc( 1, sizeof( report ));
    report_data->a = ( struct timespec * ) calloc( 1, sizeof( struct timespec ));
    report_data->b = ( struct timespec * ) calloc( 1, sizeof( struct timespec ));

    report_data->a->tv_sec = first_package_time.tv_sec - connect_time.tv_sec;
    report_data->a->tv_nsec = first_package_time.tv_nsec - connect_time.tv_nsec;

    report_data->b->tv_sec = last_package_time.tv_sec - first_package_time.tv_sec;
    report_data->b->tv_nsec = last_package_time.tv_nsec - first_package_time.tv_nsec;

    on_exit( on_exit_report, ( void * ) report_data );
}

int getData( int capacity, float download_speed, float degradation_speed, char *address, uint16_t port,
             struct sockaddr_in *local_address ) {
    struct timespec last_check_time;
    struct timespec now_time;

    struct timespec connect_time;
    struct timespec first_package_time;
    struct timespec last_package_time;

    struct timespec ts = { 0 };
    size_t sleep_ns = SMALL_PACKAGE / ( download_speed * 4435 ) * 1e9;
    ts.tv_sec = sleep_ns / 1e9;
    ts.tv_nsec = sleep_ns % ( size_t ) ( 1e9 );

    long long storage = capacity * STORAGE;
    long long actual_storage = 0;
    char buffer[SMALL_PACKAGE] = { 0 };
    while ( 1 ) {
        int soc_fd = connectToServer( address, port );
        //printf("Connected\n");

        if ( clock_gettime( CLOCK_MONOTONIC, &connect_time ) == -1 )
            errorSend( "Error during get connect time" );

        //Get data from server
        for ( int i = 0; i < 4; i++ ) {

            int recv_result = read( soc_fd, buffer, sizeof( buffer ));
            if ( recv_result == -1 )
                errorSend( "Cant read data from server" );

            if ( i == 0 )
                if ( clock_gettime( CLOCK_MONOTONIC, &first_package_time ) == -1 )
                    errorSend( "Error during get connect time" );

            if ( i == 3 )
                if ( clock_gettime( CLOCK_MONOTONIC, &last_package_time ) == -1 )
                    errorSend( "Error during get connect time" );

            //process data
            nanosleep( &ts, NULL);

            //tutaj biodegraduje material
            if ( i == 0 && actual_storage == 0 ) {
                if ( clock_gettime( CLOCK_MONOTONIC, &last_check_time ) == -1 )
                    errorSend( "Error during get connect time" );
            }
            else {
                if ( clock_gettime( CLOCK_MONOTONIC, &now_time ) == -1 )
                    errorSend( "Error during get connect time" );

                double deg = ( double ) now_time.tv_sec - ( double ) last_check_time.tv_sec +
                             ( double ) ( now_time.tv_nsec - last_check_time.tv_nsec ) * 1e-9;
                int degraded_data = ( int ) ( deg * degradation_speed * 819 );
                actual_storage -= degraded_data;

                last_check_time.tv_sec = now_time.tv_sec;
                last_check_time.tv_nsec = now_time.tv_sec;
            }

            //Processing and degradation
            actual_storage += recv_result;
        }

        generateReport( connect_time, first_package_time, last_package_time );

        if ( storage - actual_storage < FULL_PACKAGE ) {
            socklen_t addr_size = sizeof( *local_address );
            getsockname( soc_fd, ( struct sockaddr * ) local_address, &addr_size );
            close( soc_fd );
            return 0;
        }

        close( soc_fd );
    }
}

void on_exit_report( int status, void *dn ) {
    UNUSED( status );
    report *data = ( report * ) dn;
    if ( data->a->tv_nsec < 0 ) {
        data->a->tv_sec -= 1;
        data->a->tv_nsec = 1e9 - data->a->tv_nsec;
    }

    if ( data->b->tv_nsec < 0 ) {
        data->b->tv_sec -= 1;
        data->b->tv_nsec = 1e9 - data->b->tv_nsec;
    }

    fprintf( stderr, "Delay: %li sec and %li nsec\t Time: %li sec and %li nsec\n",
             data->a->tv_sec, data->a->tv_nsec, data->b->tv_sec, data->b->tv_nsec );
}

int connectToServer( char *address, uint16_t port ) {
    int client_socket = socket( AF_INET, SOCK_STREAM, 0 );
    if ( client_socket == -1 )
        errorSend( "Can't create client socket" );

    struct sockaddr_in addr;
    memset( &addr, 0, sizeof( struct sockaddr_in ));
    addr.sin_port = htons( port );
    addr.sin_family = AF_INET;
    if ( inet_aton( address, &addr.sin_addr ) == -1 )
        errorSend( "Error during parsing address" );

    if ( connect( client_socket, ( struct sockaddr * ) &addr, sizeof( addr )) == -1 )
        errorSend( "Error during connecting to server" );

    return client_socket;
}

int readInput( int argc, char *argv[], char *address, uint16_t *port, int *capacity, float *download_speed,
               float *degradation_speed ) {
    char c;
    while ( optind < argc ) {
        if (( c = ( char ) getopt( argc, argv, "c:p:d:" )) != -1 ) {
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
                default:
                    break;
            }
        }
        else {
            if ( strlen( argv[ optind ] ) <= 5 ) {
                *port = parseUInt16( argv[ optind ] );
                optind++;
            }
            else {
                *port = parsePort( argv[ optind ] );
                memcpy( address, parseAddress( argv[ optind ] ), strlen( parseAddress( argv[ optind ] )) + 1 );
                optind++;
            }
        }
    }

    return 1;
}

void errorSend( char *msg ) {
    perror( msg );
    exit( EXIT_FAILURE );
}
