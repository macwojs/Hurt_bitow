#include "producent.h"

int reserved_data = 0;
int client_count = 0;
int last_data_status = 0;


int main( int argc, char *argv[] ) {
    char address[16] = "localhost";
    uint16_t port;
    float speed;

    readInput( argc, argv, address, &port, &speed );

    int soc_fd = createServer( address, port );

    int pipe_fd = forkProduce( speed );

    int epoll_fd = createEpoll( soc_fd );

    int timer_fd = createTimer( epoll_fd );

    handleConnection( soc_fd, epoll_fd, pipe_fd, timer_fd );

    close( soc_fd );
    close( timer_fd );
    close( epoll_fd );
    close( pipe_fd );

    return 0;
}

int createTimer( int epoll_fd ) {
    int timer_fd = timerfd_create( CLOCK_MONOTONIC, TFD_NONBLOCK );
    if ( timer_fd == -1 )
        errorSend( "Can't create timer" );

    struct itimerspec ts;
    ts.it_value.tv_sec = 5;
    ts.it_value.tv_nsec = 0;
    ts.it_interval.tv_sec = 5;
    ts.it_interval.tv_nsec = 0;
    if ( timerfd_settime( timer_fd, 0, &ts, NULL) < 0 )
        errorSend( "Can't set timer" );

    struct epoll_event ev = { 0 };
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = timer_fd;

    if ( epoll_ctl( epoll_fd, EPOLL_CTL_ADD, timer_fd, &ev ) == -1 )
        errorSend( "Can't add timer descriptor to epoll" );

    return timer_fd;
}

int forkProduce( float rate ) {
    int pipe_fd[2];
    if ( pipe( pipe_fd ) == -1 )
        errorSend( "Can't create pipe" );

    int child = fork();
    if ( child == 0 ) {
        signal( SIGPIPE, SIG_IGN);
        close( pipe_fd[ 0 ] );

        produce( pipe_fd[ 1 ], rate );

        close( pipe_fd[ 1 ] );
        exit( EXIT_SUCCESS );
    }

    close( pipe_fd[ 1 ] );
    return pipe_fd[ 0 ];
}

void produce( int pipe, float rate ) {
    char data[BLOCK] = { 0 };
    char current_char = 'a';

    struct timespec ts = { 0 };
    size_t sleep_ns = BLOCK / ( rate * 2662 ) * 1e9;
    ts.tv_sec = sleep_ns / 1e9;
    ts.tv_nsec = sleep_ns % ( size_t ) ( 1e9 );

    for ( ;; ) {
        for ( size_t i = 0; i < BLOCK; ++i ) {
            data[ i ] = current_char;
        }

        int write_data = write( pipe, data, BLOCK );
        if ( write_data == -1 ) {
            if ( errno == EPIPE ) {
                return;
            }
            errorSend( "Can't write data to pipe" );
        }

        if ( current_char == 'z' )
            current_char = 'A';
        else if ( current_char == 'Z' )
            current_char = 'a';
        else
            ++current_char;

        nanosleep( &ts, NULL);
    }
}

void addToEpoll( int epoll_fd, int cl_fd ) {
    struct epoll_event ev = { 0 };

    ev.events = EPOLLOUT | EPOLLRDHUP;

    struct socket_data *data = ( socket_data * ) calloc( 1, sizeof( socket_data ));
    data->fd = cl_fd;
    data->data_to_send = FULL_PACKAGE;

    ev.data.ptr = data;

    if ( epoll_ctl( epoll_fd, EPOLL_CTL_ADD, cl_fd, &ev ) == -1 )
        errorSend( "Can't add new descriptor to epoll" );
}

void connectNewClient( int cl_fd, int epoll_fd, int pipe_fd, list *quote ) {
    int nbytes = 0;
    ioctl( pipe_fd, FIONREAD, &nbytes );
    if ( nbytes - reserved_data > FULL_PACKAGE && quote->value == -1 ) {
        addToEpoll( epoll_fd, cl_fd );
    }
    else {
        put( quote, cl_fd );
    }
}

void disconnectClient( socket_data *data, int epoll_fd, int pipe_fd ) {
    struct timespec time_stamp;

    struct sockaddr_in address = { 0 };
    socklen_t addressLength = sizeof( address );
    if ( getpeername( data->fd, ( struct sockaddr * ) &address, &addressLength ) == -1 )
        errorSend( "Can't get address from descriptor" );

    shutdown( data->fd, SHUT_RDWR );
    client_count--;

    if ( epoll_ctl( epoll_fd, EPOLL_CTL_DEL, data->fd, NULL) == -1 )
        errorSend( "Can't remove descriptor from epoll" );

    close( data->fd );

    if ( clock_gettime( CLOCK_REALTIME, &time_stamp ) == -1 )
        errorSend( "Error during get disconnect time" );

    fprintf( stderr, "\nClient disconnected  time: %li sec, %li nsec\n", time_stamp.tv_sec,
             time_stamp.tv_nsec );
    fprintf( stderr, "\t\t\t\t\t address: %s:%d\n", inet_ntoa( address.sin_addr ), ntohs( address.sin_port ));
    fprintf( stderr, "\t\t\t\t\t lost data: %d\n", data->data_to_send );

    if ( data->data_to_send > 0 ) {
        char read_buffer[data->data_to_send];
        if ( read( pipe_fd, read_buffer, sizeof( read_buffer )) == -1 )
            errorSend( "Can't read data from pipe" );

        reserved_data -= data->data_to_send;
    }

    free( data );
}

void sendData( socket_data *data, int epoll_fd, int pipe_fd ) {
    if ( data->data_to_send == FULL_PACKAGE ) {
        reserved_data += FULL_PACKAGE;
    }

    char read_buffer[SMALL_PACKAGE] = { 0 };
    int read_pipe = read( pipe_fd, read_buffer, sizeof( read_buffer ));
    if ( read_pipe == -1 )
        errorSend( "Can't read data from pipe" );

    reserved_data -= SMALL_PACKAGE; //Pobralismy dane z pipe, wiec jak znikna to sie zmarnuja

    int write_sock = write( data->fd, read_buffer, sizeof( read_buffer ));
    if ( write_sock == -1 )
        disconnectClient( data, epoll_fd, pipe_fd );

    //printf( "New data sent\n" );

    //update events setting
    data->data_to_send -= SMALL_PACKAGE;

    struct epoll_event ev = { 0 };
    ev.events = EPOLLOUT | EPOLLRDHUP;
    ev.data.ptr = data;

    if ( epoll_ctl( epoll_fd, EPOLL_CTL_MOD, data->fd, &ev ) == -1 )
        errorSend( "Can't add new descriptor to epoll" );

    if ( data->data_to_send == 0 ) {
        disconnectClient( data, epoll_fd, pipe_fd );
    }
}

void addFromQuote( int epoll_fd, list **quote, int pipe_fd ) {
    int nbytes;
    ioctl( pipe_fd, FIONREAD, &nbytes );
    if ( nbytes - reserved_data > FULL_PACKAGE ) {
        for ( int i = 0; i < ( nbytes - reserved_data ) / FULL_PACKAGE; i++ ) {
            int fd = get( *quote );
            if ( fd != -1 ) {
                addToEpoll( epoll_fd, fd );
                *quote = del( *quote );
            }
        }
    }
}

void timerReport( int timer_fd, int pipe_fd ) {
    const size_t pipe_capacity = fcntl( pipe_fd, F_GETPIPE_SZ );
    int pipe_current;
    uint64_t temp_buf;
    if ( read( timer_fd, &temp_buf, sizeof( temp_buf )) == -1 ) {
        printf( "err read\n" );
    }
    struct timespec time_stamp;
    if ( clock_gettime( CLOCK_REALTIME, &time_stamp ) == -1 )
        errorSend( "Error during get disconnect time" );

    fprintf( stderr, "\nReport for time: %li sec, %li nsec\n", time_stamp.tv_sec,
             time_stamp.tv_nsec );
    ioctl( pipe_fd, FIONREAD, &pipe_current );
    fprintf( stderr, "\t\t\t\t Ilosc klientow: %d\n", client_count );
    fprintf( stderr, "\t\t\t\t Zajetosc magazynu: %d (%.2f%%)\n", pipe_current,
             ( float ) pipe_current / pipe_capacity * 100 );
    fprintf( stderr, "\t\t\t\t Przeplyw: %d\n", pipe_current - last_data_status );
    last_data_status = pipe_current;
}

void handleConnection( int soc_fd, int epoll_fd, int pipe_fd, int timer_fd ) {
    int ready;

    struct epoll_event *evlist = calloc(EPOLL_WAIT_LIMIT, sizeof( struct epoll_event ));

    list *quote = create();

    while ( 1 ) {
        addFromQuote( epoll_fd, &quote, pipe_fd );

        ready = epoll_wait( epoll_fd, evlist, EPOLL_WAIT_LIMIT, 0 );
        if ( ready == -1 ) {
            if ( errno == EINTR )
                continue;
            else
                errorSend( "Error during epoll wait" );
        }

        for ( int i = 0; i < ready; ++i ) {
            if ( evlist[ i ].data.fd == soc_fd ) {
                int client_fd = accept( soc_fd, NULL, NULL);
                client_count++;
                connectNewClient( client_fd, epoll_fd, pipe_fd, quote );
            }
            else if ( evlist[ i ].data.fd == timer_fd && evlist[ i ].events & EPOLLIN ) {
                timerReport( timer_fd, pipe_fd );
            }
            else if ( evlist[ i ].events & EPOLLRDHUP ) {
                struct socket_data *data = ( struct socket_data * ) evlist[ i ].data.ptr;
                if ( data->data_to_send ==
                     FULL_PACKAGE )  //transmisja sie nie rozpoczela, wiec dane nie zostaly utracone
                    data->data_to_send = 0;
                disconnectClient( data, epoll_fd, pipe_fd );
            }
            else if ( evlist[ i ].events & EPOLLOUT ) {
                struct socket_data *data = ( struct socket_data * ) evlist[ i ].data.ptr;
                sendData( data, epoll_fd, pipe_fd );
            }
        }
    }
}

int createEpoll( int soc_fd ) {
    int epoll_fd = epoll_create1( 0 );
    if ( epoll_fd == -1 )
        errorSend( "Can't create epoll" );

    struct epoll_event ev = { 0 };
    ev.events = EPOLLIN;
    ev.data.fd = soc_fd;

    if ( epoll_ctl( epoll_fd, EPOLL_CTL_ADD, soc_fd, &ev ) == -1 )
        errorSend( "Can't add descriptor to epoll" );

    return epoll_fd;
}

int createServer( char *address, uint16_t port ) {
    int serverSocket = socket( AF_INET, SOCK_STREAM, 0 );

    if ( serverSocket == -1 )
        errorSend( "Can't create server socket" );

    struct sockaddr_in addr;
    memset( &addr, 0, sizeof( struct sockaddr_in ));
    addr.sin_port = htons( port );
    addr.sin_family = AF_INET;
    int res = inet_aton( address, &addr.sin_addr );

    if ( res == -1 )
        errorSend( "Error int inet_aton" );

    if ( bind( serverSocket, ( struct sockaddr * ) &addr, sizeof( struct sockaddr_in )) == -1 )
        errorSend( "Can't bind server socket" );

    if ( listen( serverSocket, 20 ) == -1 )
        errorSend( "Error in listen" );

    return serverSocket;
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
