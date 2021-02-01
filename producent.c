#include "producent.h"

int main( int argc, char *argv[] ) {
    char address[16] = "localhost";
    uint16_t port = 5566;
    float speed;

    readInput( argc, argv, address, &port, &speed );

    int soc_fd = createServer( address, port );

    int epoll_fd = createEpoll( soc_fd );

    int pipe_fd = forkProduce();

    handleConnection( soc_fd, epoll_fd, pipe_fd );

    close( soc_fd );
    close( epoll_fd );
    close( pipe_fd );

    return 0;
}

int forkProduce() {
    int pipe_fd[2];
    if ( pipe( pipe_fd ) == -1 ) {
        perror( "Can't create pipe" );
        exit( EXIT_FAILURE );
    }

    int child = fork();
    if ( child == 0 ) {
        //TODO: obsluga magazynu
    }

    close( pipe_fd[ 1 ] );
    return pipe_fd[ 0 ];
}

void handleConnection( int soc_fd, int epoll_fd, int pipe_fd ) {
    int ready;
    struct epoll_event *evlist = calloc(EPOLL_WAIT_LIMIT, sizeof( struct epoll_event ));
    struct epoll_event ev = {};

    while ( 1 ) {
        ready = epoll_wait( epoll_fd, evlist, EPOLL_WAIT_LIMIT, -1 );
        if ( ready == -1 ) {
            if ( errno == EINTR )
                continue;
            else {
                perror( "Error during epoll wait" );
                exit( EXIT_FAILURE );
            }
        }

        for ( int i = 0; i < ready; ++i ) {
            if ( evlist[ i ].data.u32 == 15 ) {
                struct sockaddr_in client_address;
                uint32_t client_size = sizeof( client_address );
                int client_fd = accept( soc_fd, ( struct sockaddr * ) &client_address, &client_size );
                ev.events = EPOLLIN;
                ev.data.ptr = &client_address;
                ev.data.fd = client_fd;
                if ( epoll_ctl( epoll_fd, EPOLL_CTL_ADD, client_fd, &ev ) == -1 ) {
                    perror( "Can't add new descriptor to epoll" );
                    exit( EXIT_FAILURE );
                }
            } else if ( evlist[ i ].events & EPOLLIN ) {
                char trashBuf[64] = {};
                int recvRes;
                do { //clear buffer
                    recvRes = recv( evlist[ i ].data.fd, trashBuf, sizeof( trashBuf ), MSG_DONTWAIT );
                } while ( recvRes > 0 );
                if ( recvRes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK )) {
                    errno = 0;
                } else {
                    perror( "Cant read data from client" );
                    exit( EXIT_FAILURE );
                }

                //Wait for full storage
                int nbytes = 0;
                do {
                    ioctl( pipe_fd, FIONREAD, &nbytes );
                } while ( nbytes < 13312 );

                char read_buffer[13312] = {};
                int read_pipe = read( pipe_fd, read_buffer, sizeof( read_buffer ));
                if ( read_pipe == -1 ) {
                    perror( "Can't read data from pipe" );
                    exit( EXIT_FAILURE );
                }

                int write_sock = write( evlist[ i ].data.fd, read_buffer, sizeof( read_buffer ));
                if ( write_sock == -1 ) {
                    //TODO: Handle disconected client
                } else if ( write_sock < 13312 ){
                    //TODO: Handle disconected client
                }

            } else if ( evlist[ i ].events & EPOLLHUP ) {
                //TODO: connection droped
            }
        }
    }

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
    ev.data.u32 = 15;
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
        } else {
            *port = parsePort( argv[ optind ] );
            memcpy( address, parseAddress( argv[ optind ] ), strlen( parseAddress( argv[ optind ] )) + 1 );
            optind++;
        }
    }

    return 1;
}