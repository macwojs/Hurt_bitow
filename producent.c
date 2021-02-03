#include "producent.h"

typedef struct sockaddr sockaddr;
long long reserved_data = 0;

int main( int argc, char *argv[] ) {
    char address[16] = "localhost";
    uint16_t port;
    float speed;

    readInput( argc, argv, address, &port, &speed );

    int soc_fd = createServer( address, port );

    int epoll_fd = createEpoll( soc_fd );

    int pipe_fd = forkProduce( speed );

    //TODO: timer

    handleConnection( soc_fd, epoll_fd, pipe_fd, speed );

    close( soc_fd );
    close( epoll_fd );
    close( pipe_fd );

    return 0;
}

int forkProduce( float rate ) {
    int pipe_fd[2];
    if ( pipe( pipe_fd ) == -1 ) {
        perror( "Can't create pipe" );
        exit( EXIT_FAILURE );
    }

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
    char data[640] = {};
    char current_char = 'a';

    struct timespec ts = {};
    size_t sleep_ns = 640 / ( rate * 2662 ) * 1e9;
    ts.tv_sec = sleep_ns / 1e9;
    ts.tv_nsec = sleep_ns % ( size_t ) ( 1e9 );

    for ( ;; ) {
        for ( size_t i = 0; i < 640; ++i ) {
            data[ i ] = current_char;
        }

        //TODO: sprawdz czy pip nie jest pelny

        int write_data = write( pipe, data, 640 );
        if ( write_data == -1 ) {
            if ( errno == EPIPE ) {
                return;
            }
            perror( "Can't write data to pipe" );
            exit( EXIT_FAILURE );
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
    struct epoll_event ev = {};

    ev.events = EPOLLOUT | EPOLLRDHUP;

    struct socket_data *data = ( socket_data * ) calloc( 1, sizeof( socket_data ));
    data->fd = cl_fd;
    data->data_to_send = FULL_PACKAGE;

    ev.data.ptr = data;

    if ( epoll_ctl( epoll_fd, EPOLL_CTL_ADD, cl_fd, &ev ) == -1 ) {
        perror( "Can't add new descriptor to epoll" );
        exit( EXIT_FAILURE );
    }
}

void connectNewClient( int cl_fd, int epoll_fd, int pipe_fd, list *quote ) {
    int nbytes = 0;
    ioctl( pipe_fd, FIONREAD, &nbytes );
    if ( nbytes - reserved_data > 13312 ) {
        addToEpoll( epoll_fd, cl_fd );
    }
    else {
        put( quote, cl_fd );
    }
}

void disconnectClient( socket_data *data, int epoll_fd ) {
    struct timespec time_stamp;

    struct sockaddr_in address = { 0 };
    socklen_t addressLength = sizeof( address );
    if ( getpeername( data->fd, ( struct sockaddr * ) &address, &addressLength ) == -1 ) {
        perror( "Can't get address from descriptor" );
        exit( EXIT_FAILURE );
    }

    shutdown( data->fd, SHUT_RDWR );

    if ( epoll_ctl( epoll_fd, EPOLL_CTL_DEL, data->fd, NULL) == -1 ) {
        perror( "Can't remove descriptor from epoll" );
        exit( EXIT_FAILURE );
    }

    close( data->fd );

    if ( clock_gettime( CLOCK_REALTIME, &time_stamp ) == -1 ) {
        perror( "Error during get disconnect time" );
        exit( EXIT_FAILURE );
    }

    fprintf( stderr, "Client disconnected  time: %li sec, %li nsec\n", time_stamp.tv_sec,
             time_stamp.tv_nsec );
    fprintf( stderr, "\t\t\t\t\t address: %s:%d\n", inet_ntoa( address.sin_addr ), ntohs( address.sin_port ));
    fprintf( stderr, "\t\t\t\t\t lost data: %d\n", data->data_to_send );

    free( data );
}

void sendData( socket_data *data, int epoll_fd, int pipe_fd ) {
    if ( data->data_to_send == FULL_PACKAGE ) {
        reserved_data += FULL_PACKAGE;
    }

    char read_buffer[SMALL_PACKAGE] = {};
    int read_pipe = read( pipe_fd, read_buffer, sizeof( read_buffer ));
    if ( read_pipe == -1 ) {
        perror( "Can't read data from pipe" );
        exit( EXIT_FAILURE );
    }

    reserved_data -= SMALL_PACKAGE; //Pobralismy dane z pipe, wiec jak znikna to sie zmarnuja

    int write_sock = write( data->fd, read_buffer, sizeof( read_buffer ));
    if ( write_sock == -1 ) {
        perror( "Can't send data to client pipe" );
        exit( EXIT_FAILURE );
    }

    printf( "New data sent\n" );

    //TODO: sprawdz, czy wszystko sie wyslalo, a jak nie to czy klient nie umarla, a jak umarl to go usun i zrob raport

    //update events setting
    data->data_to_send -= SMALL_PACKAGE;

    struct epoll_event ev = {};
    ev.events = EPOLLOUT | EPOLLRDHUP;
    ev.data.ptr = data;

    if ( epoll_ctl( epoll_fd, EPOLL_CTL_MOD, data->fd, &ev ) == -1 ) {
        perror( "Can't add new descriptor to epoll" );
        exit( EXIT_FAILURE );
    }

    if ( data->data_to_send == 0 ) {
        disconnectClient( data, epoll_fd );
    }
}

void handleConnection( int soc_fd, int epoll_fd, int pipe_fd, float rate ) {
    int timeout = ( int ) (( 640 / ( rate * 2662 )) * 1e3 ); //production rate in ms for epoll timeout

    int ready, nbytes;

    struct epoll_event *evlist = calloc(EPOLL_WAIT_LIMIT, sizeof( struct epoll_event ));

    list *quote = create();

    while ( 1 ) {
        ioctl( pipe_fd, FIONREAD, &nbytes );
        if ( nbytes - reserved_data > 13312 ) {
            for ( int i = 0; i < ( nbytes - reserved_data ) / 13312; i++ ) {
                int fd = get( quote );
                if ( fd != -1 ) {
                    addToEpoll( epoll_fd, fd );
                    quote = del( quote );
                }
            }
        }

        ready = epoll_wait( epoll_fd, evlist, EPOLL_WAIT_LIMIT, timeout );
        if ( ready == -1 ) {
            if ( errno == EINTR )
                continue;
            else {
                perror( "Error during epoll wait" );
                exit( EXIT_FAILURE );
            }
        }

        for ( int i = 0; i < ready; ++i ) {
            if ( evlist[ i ].data.fd == soc_fd ) {
                printf( "New client connected\n" );
                int client_fd = accept( soc_fd, NULL, NULL);
                connectNewClient( client_fd, epoll_fd, pipe_fd, quote );
                printf( "New client properly connected\n" );

            }
            else if ( evlist[ i ].events & EPOLLRDHUP ) {
                printf( "Client disconnecting\n" );
                struct socket_data *data = ( struct socket_data * ) evlist[ i ].data.ptr;
                disconnectClient( data, epoll_fd );
                printf( "Client disconnected\n" );
            }
            else if ( evlist[ i ].events & EPOLLOUT ) {
                printf( "New data request\n" );
                struct socket_data *data = ( struct socket_data * ) evlist[ i ].data.ptr;
                sendData( data, epoll_fd, pipe_fd );
            }
        }
    }

    free( quote );
    free( evlist );
}

int createEpoll( int soc_fd ) {
    int epoll_fd = epoll_create1( 0 );
    if ( epoll_fd == -1 ) {
        perror( "Can't create epoll" );
        exit( EXIT_FAILURE );
    }

    struct epoll_event ev = {};
    ev.events = EPOLLIN;
    ev.data.fd = soc_fd;
    //ev.data.u32 = 15;
    if ( epoll_ctl( epoll_fd, EPOLL_CTL_ADD, soc_fd, &ev ) == -1 ) {
        perror( "Can't add descriptor to epoll" );
        exit( EXIT_FAILURE );
    }

    return epoll_fd;
}

int createServer( char *address, uint16_t port ) {
    int serverSocket = socket( AF_INET, SOCK_STREAM, 0 );

    if ( serverSocket == -1 ) {
        perror( "Can't create server socket\n" );
        exit( EXIT_FAILURE );
    }

    struct sockaddr_in addr;
    memset( &addr, 0, sizeof( struct sockaddr_in ));
    addr.sin_port = htons( port );
    addr.sin_family = AF_INET;
    int res = inet_aton( address, &addr.sin_addr );

    if ( res == -1 ) {
        perror( "Error int inet_aton\n" );
        exit( EXIT_FAILURE );
    }

    if ( bind( serverSocket, ( struct sockaddr * ) &addr, sizeof( struct sockaddr_in )) == -1 ) {
        perror( "Can't bind server socket\n" );
        exit( EXIT_FAILURE );
    }

    if ( listen( serverSocket, 20 ) == -1 ) {
        perror( "Error in listen\n" );
        exit( EXIT_FAILURE );
    }

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