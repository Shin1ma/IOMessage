/*
    made by Ruben amadei on the xx/xx/2026
*/

#include "../../NetworkManager.h"

NM_srv_context_t* NM_init_server(const char* port){
    int err, yes=1;
    NM_srv_context_t* ctx = malloc(sizeof(NM_srv_context_t));
    if(!ctx)
        abort_error("failed to alloc context for NetworkManager, something is very wrong");

    //context init with port and ip copying
    strncpy(ctx->port.string_port, port, PORT_LEN_MAX);
    ctx->port.string_port[PORT_LEN_MAX-1] = '\0';
    ctx->port.num_port = atoi(ctx->port.string_port);
    strncpy(ctx->string_ip, LOOPBACK, IP_LEN_MAX);
    ctx->string_ip[IP_LEN_MAX-1] = '\0';

    //starting up WSA
    if(WSAStartup(MAKEWORD(2,2), &(ctx->winsock_data)) != 0){
        free(ctx);
        abort_error("WSA startup failed");
    }
    //checking if the version is available
    if(LOBYTE(ctx->winsock_data.wVersion) != 2 || HIBYTE(ctx->winsock_data.wVersion) != 2){
        free(ctx);
        WSACleanup();
        abort_error("WSA 2.2 not available");
    }

    //getting addrinfo struct
    struct addrinfo hints = {0};
    hints.ai_flags = AI_PASSIVE;    //get my ip
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if((err = getaddrinfo(NULL, ctx->port.string_port, &hints, &ctx->addr)) != 0){
        free(ctx);
        abort_error(gai_strerror(err));
    }
    
    //creating and setting up socket
    ctx->n_sock = socket(ctx->addr->ai_family, ctx->addr->ai_socktype, ctx->addr->ai_protocol);
    if(ctx->n_sock == SOCKET_ERROR){
        err = WSAGetLastError();
        freeaddrinfo(ctx->addr);
        free(ctx);
        WSACleanup();
        abort_numerror("failed to create socket with error: ", err);
    }
    if(setsockopt(ctx->n_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof yes) == SOCKET_ERROR)
        numerror("setsockopt failed (port may not be reusable without restart), with error: ", WSAGetLastError());
    if(bind(ctx->n_sock, ctx->addr->ai_addr, (int)ctx->addr->ai_addrlen) == SOCKET_ERROR){
        err = WSAGetLastError();
        freeaddrinfo(ctx->addr);
        closesocket(ctx->n_sock);
        free(ctx);
        WSACleanup();
        abort_numerror("bind failed with provided port, error: ", err);
    }
    if(listen(ctx->n_sock, BACKLOG) == SOCKET_ERROR){   //setting it as a listening socket
        err = WSAGetLastError();
        freeaddrinfo(ctx->addr);
        closesocket(ctx->n_sock);
        free(ctx);
        WSACleanup();
        abort_numerror("listen failed, exit code: ", err);
    } 

    return ctx;
}
NM_client_context_t* NM_init_client(const char* dst_port, const char* src_port, const char* ip){
    int err, yes=1;
    NM_client_context_t* ctx = malloc(sizeof(NM_client_context_t));
    if(!ctx)
        abort_error("failed to alloc context for NetworkManager, something is very wrong");

    //context init with port and ip copying for src
    strncpy(ctx->port.string_port, src_port, PORT_LEN_MAX);
    ctx->port.string_port[PORT_LEN_MAX-1] = '\0';
    ctx->port.num_port = atoi(ctx->port.string_port);
    strncpy(ctx->string_ip, LOOPBACK, IP_LEN_MAX);
    ctx->string_ip[IP_LEN_MAX-1] = '\0';

    //making the destination a connection to the socket
    ctx->connection = malloc(sizeof(NM_conn_t));
    if(!ctx->connection){
        free(ctx);
        abort_error("connection allocation for client purposes failed");
    }
    strncpy(ctx->connection->port.string_port, dst_port, PORT_LEN_MAX);
    ctx->connection->port.string_port[PORT_LEN_MAX-1] = '\0';
    ctx->connection->port.num_port = atoi(ctx->connection->port.string_port);
    strncpy(ctx->connection->string_ip, ip, IP_LEN_MAX);
    ctx->connection->string_ip[IP_LEN_MAX-1] = '\0';

    //starting up WSA
    if(WSAStartup(MAKEWORD(2,2), &(ctx->winsock_data)) != 0){
        free(ctx);
        abort_error("WSA startup failed");
    }
    //checking if the version is available
    if(LOBYTE(ctx->winsock_data.wVersion) != 2 || HIBYTE(ctx->winsock_data.wVersion) != 2){
        free(ctx->connection);
        free(ctx);
        WSACleanup();
        abort_error("WSA 2.2 not available");
    }

    //getting addrinfo struct for src
    struct addrinfo hints = {0};
    hints.ai_flags = AI_PASSIVE; //get my ip
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if((err = getaddrinfo(NULL, ctx->port.string_port, &hints, &ctx->src_addr)) != 0){
        free(ctx->connection);
        free(ctx);
        WSACleanup();
        abort_error(gai_strerror(err));
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if((err = getaddrinfo(ctx->connection->string_ip, ctx->connection->port.string_port, &hints, &ctx->dest_addr)) != 0){
        free(ctx->connection);
        freeaddrinfo(ctx->src_addr);
        free(ctx);
        WSACleanup();
        abort_error(gai_strerror(err));
    }
    memcpy((struct sockaddr*)&ctx->connection->accepted_addr, ctx->dest_addr->ai_addr, sizeof(struct sockaddr));
    
    //creating and setting up socket
    ctx->n_sock = socket(ctx->src_addr->ai_family, ctx->src_addr->ai_socktype, ctx->src_addr->ai_protocol);
    if(ctx->n_sock == SOCKET_ERROR){
        err = WSAGetLastError();
        freeaddrinfo(ctx->src_addr);
        free(ctx->connection);
        free(ctx);
        WSACleanup();
        abort_numerror("failed to create socket with error: ", err);
    }
    ctx->connection->n_sock = ctx->n_sock;
    if(setsockopt(ctx->n_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof yes) == SOCKET_ERROR)
        numerror("setsockopt failed (port may not be reusable without restart), with error: ", WSAGetLastError());
    if(bind(ctx->n_sock, ctx->src_addr->ai_addr, (int)ctx->src_addr->ai_addrlen) == SOCKET_ERROR){
        err = WSAGetLastError();
        freeaddrinfo(ctx->src_addr);
        freeaddrinfo(ctx->dest_addr);
        free(ctx->connection);
        closesocket(ctx->n_sock);
        free(ctx);
        WSACleanup();
        abort_numerror("bind failed with provided port, error: ", err);
    }
    if(connect(ctx->n_sock, ctx->dest_addr->ai_addr, sizeof(struct sockaddr)) == SOCKET_ERROR){
        err = WSAGetLastError();
        freeaddrinfo(ctx->src_addr);
        freeaddrinfo(ctx->dest_addr);
        free(ctx->connection);
        closesocket(ctx->n_sock);
        free(ctx);
        WSACleanup();
        abort_numerror("accept failed with provided port and ip, error: ", err);
    }

    return ctx;
}

NM_conn_t* NM_accept_conn(const NM_srv_context_t* ctx){
    NM_conn_t* connected_sock = malloc(sizeof(NM_conn_t));    //accept and create a connection struct
    if(!connected_sock){
        error("couldn't allocate for new connection");
        return NULL;
    }
    int sockaddr_in_len = sizeof(connected_sock->accepted_addr);
    //initialize the connected socket
    if((connected_sock->n_sock = accept(ctx->n_sock, (struct sockaddr*)&(connected_sock->accepted_addr), &sockaddr_in_len)) == SOCKET_ERROR){
        free(connected_sock);
        numerror("accept failed, exit code: ", WSAGetLastError());
        return NULL;
    }

    //translate ip to presentation and fill out port and ip
    if(inet_ntop(AF_INET, &(connected_sock->accepted_addr.sin_addr), connected_sock->string_ip, IP_LEN_MAX) == NULL){
        numerror("failed to presentate ip into string format, substituting err, fail num: ", WSAGetLastError());
        snprintf(connected_sock->string_ip, IP_LEN_MAX, "ERR");
    }
    connected_sock->port.num_port = ntohs(connected_sock->accepted_addr.sin_port);
    snprintf(connected_sock->port.string_port, PORT_LEN_MAX, "%d", connected_sock->port.num_port);

    return connected_sock;
}
int NM_send_all(const void* buf, size_t len, int timeout, NM_conn_t* conn){
    int sent = 0;
    int timeout_reset = 0;
    int sent_now;
    int err;

    if(timeout != NO_TIMEOUT){  //set timer
        if(setsockopt(conn->n_sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout)) == SOCKET_ERROR){
            error("timeout set failed, socket may hang forever during send");
        }
    }

    //send loop to send all bytes
    while(sent < len){
        int remaining = (int)len-sent;
        sent_now = send(conn->n_sock, (char*)buf+sent, remaining, 0);
        if(sent_now == SOCKET_ERROR){
            err = WSAGetLastError();
            if(err == WSAEINTR) 
                continue;
            sent = -1;
            switch(err){    //system independant error codes
                case WSAENOTCONN:
                case WSAECONNRESET:
                    NM_set_error(NM_CONN_CLOSED, conn);
                    break;
                case WSAEWOULDBLOCK:
                    NM_set_error(NM_WOULDBLOCK, conn);
                    break;
                case WSAETIMEDOUT:
                    NM_set_error(NM_TIMEDOUT, conn);
                    break;
                default:
                    NM_set_error(NM_UNKNOWN_ERROR, conn);
                    break;
            }
            numerror("error while sending: ", NM_get_error(conn));
            sent = -1;
            break;
        }
        if(sent_now == 0){
            sent = -1;
            NM_set_error(NM_CONN_CLOSED, conn);
            break;
        }
        sent+=sent_now;
    }
    if(timeout != NO_TIMEOUT){  //reset timeout
        if(setsockopt(conn->n_sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout_reset, sizeof(timeout_reset)) == SOCKET_ERROR){
            error("timeout reset failed");
        }
    }
    return sent;
}

int NM_recv(char* buf, int len, int timeout, NM_conn_t* conn){
    int reset_timeout = 0;
    int received;
    int err;
    if(timeout != NO_TIMEOUT){  //set timeout
        if(setsockopt(conn->n_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) == SOCKET_ERROR){
            error("timeout set failed, socket may hang forever during recv");
        }
    }
    received = recv(conn->n_sock, buf, len, 0);

    //system independant error codes
    if(received > 0) NM_set_error(NM_OK, conn); 
    else if(received == 0) NM_set_error(NM_CONN_CLOSED, conn);
    else{
        err = WSAGetLastError();
        switch(err){
            case WSAENOTCONN:
            case WSAECONNRESET:
                NM_set_error(NM_CONN_CLOSED, conn);
                break;
            case WSAEWOULDBLOCK:
                NM_set_error(NM_WOULDBLOCK, conn);
                break;
            case WSAETIMEDOUT:
                NM_set_error(NM_TIMEDOUT, conn);
                break;
            case WSAEINTR:
                NM_set_error(NM_INTERRUPTED, conn);
                break;
            default:
                NM_set_error(NM_UNKNOWN_ERROR, conn);
                break;
        }
    }
    if(timeout != NO_TIMEOUT){ //reset timeout
        if(setsockopt(conn->n_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&reset_timeout, sizeof(reset_timeout)) == SOCKET_ERROR){
            error("timeout reset failed");
        }
    }

    return received;
}

int NM_get_error(NM_conn_t* conn){
    int err = conn->error;
    conn->error = NM_OK;
    return err;
}
void NM_set_error(int err, NM_conn_t* conn){
    conn->error = err;
}

void NM_destroy_conn(NM_conn_t* conn){
    shutdown(conn->n_sock, SD_BOTH);
    closesocket(conn->n_sock);
    free(conn);
}
void NM_srv_destroy(NM_srv_context_t* ctx){
    freeaddrinfo(ctx->addr);
    closesocket(ctx->n_sock);
    free(ctx);
    WSACleanup();
}
void NM_client_destroy(NM_client_context_t* ctx){
    freeaddrinfo(ctx->dest_addr);
    freeaddrinfo(ctx->src_addr);
    closesocket(ctx->n_sock);
    free(ctx->connection);
    free(ctx);
    WSACleanup();
}