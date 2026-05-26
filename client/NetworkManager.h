/*
made by Ruben Amadei on xx/xx/xxxx

abstraction layer for OS networking
*/

#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#ifdef _WIN32
#elif defined(__linux__)
#else
    #error "system not supported"
#endif

#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
#elif defined(__linux__)

#endif
#include "utils.h"

#define LOOPBACK "127.0.0.1"
#define PORT_LEN_MAX    6
#define IP_LEN_MAX  16
#define BACKLOG 3
#define CHUNK_LEN_SND   200
#define CHUNK_LEN_RCV   50

enum NM_comm_errs{ //to make implementation independant errors
    NM_OK = 0,
    NM_CONN_CLOSED,
    NM_WOULDBLOCK,
    NM_TIMEDOUT,
    NM_INTERRUPTED,
    NM_UNKNOWN_ERROR
};

enum NM_EnumTimeout{
    NO_TIMEOUT = 0
};



//to generalize socket types
#ifdef _WIN32
    typedef SOCKET NM_socket_t;
#elif defined(__linux__)
    typedef int NM_socket_t;
#endif


typedef struct NM_srv_util_port{ 
    char string_port[PORT_LEN_MAX];
    int num_port;
} NM_srv_port_t;    //port that contains number and string rapresentation
typedef struct NM_conn{   
    NM_socket_t n_sock;
    struct sockaddr_in accepted_addr;
    char string_ip[IP_LEN_MAX];
    struct NM_srv_util_port port;
    int error;
} NM_conn_t;    //stores info about accepted connections
typedef struct NM_client_context{ 
    NM_socket_t n_sock;
    struct addrinfo* dest_addr;
    struct addrinfo* src_addr;
    char string_ip[IP_LEN_MAX];
    NM_srv_port_t port;
    NM_conn_t* connection;
    #ifdef _WIN32
        struct WSAData winsock_data;
    #endif
} NM_client_context_t; //handles context
typedef struct NM_srv_context{ 
    NM_socket_t n_sock;
    struct addrinfo* addr;
    char string_ip[IP_LEN_MAX];
    struct NM_srv_util_port port;
    #ifdef _WIN32
        struct WSAData winsock_data;
    #endif
} NM_srv_context_t; //handles context


NM_srv_context_t* NM_init_server(const char* port);    //init network manager for servers
NM_client_context_t* NM_init_client(const char* dst_port, const char* src_port, const char* ip);    //init network manager for client
NM_conn_t* NM_accept_conn(const NM_srv_context_t* ctx);   //accept a connection [BLOCKING]
int NM_send_all(const void* buf, size_t len, int timeout, NM_conn_t* conn);    //send all bytes
int NM_recv(char* buf, int len, int timeout, NM_conn_t* conn);  //receive wrapper
int NM_get_error(NM_conn_t* conn);  //simple get set wrappers
void NM_set_error(int err, NM_conn_t* conn);
void NM_destroy_conn(NM_conn_t* conn); //destroys a connection
void NM_srv_destroy(NM_srv_context_t* ctx);    //closes network manager
void NM_client_destroy(NM_client_context_t* ctx);    //closes network manager

#endif