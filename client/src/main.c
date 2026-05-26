#include <stdio.h>
#include <time.h>

#include "../NetworkManager.h"
#include "../utils.h"

#define MAX_IP_STR  17
#define MAX_PORT_STR    6
#define REGISTERED_PORTS    49151
#define MAX_PORT_NUM    65535



int main(int argc, char* argv[]){
    if(argc < 3){
        abort_error("usage: client <server ip> <server port>");
    }

    char ip[MAX_IP_STR];
    char src_portstr[MAX_PORT_STR];
    char dst_portstr[MAX_PORT_STR];
    unsigned short dst_port;
    unsigned short src_port;

    unsigned int time_ms = (unsigned int)time(NULL);
    srand(time_ms);

    strncpy(ip, argv[1], MAX_IP_STR);
    dst_port = atoi(argv[2]);
    if(dst_port < 1024 || dst_port > REGISTERED_PORTS) abort_error("server port is invalid or well known");
    strncpy(dst_portstr, argv[2], MAX_PORT_STR);
    src_port = (rand() + REGISTERED_PORTS) % MAX_PORT_NUM;
    snprintf(src_portstr, MAX_PORT_STR, "%hu", src_port);

    NM_client_context_t* ctx;


    ctx = NM_init_client(dst_portstr, src_portstr, ip);
    if(!ctx){
        abort_error("failed to create context");
    }
    
    char* msg = NULL;
    char* type;
    char* params;
    char* payload;
    char ch;
    while(1){
        if(msg != NULL){
            printf("same as before (y, n)?\n");
            scanf(" %c", &ch);
        }
        else ch = 'n';
        if(ch == 'n'){
            free(msg);
            printf("insert type: ");
            scanf(" ");
            if(!(type = input_string(20))) return 1;
            printf("\n");
            printf("insert params: ");
            if(!(params = input_string(20))) return 1;
            printf("\n");
            printf("insert payload: ");
            if(!(payload = input_string(20))) return 1;
            printf("\n");

            size_t f_size = snprintf(NULL, 0, "[{%s}{%s}{%s}]", type, params, payload);
            msg = malloc(f_size+1);
            if(!msg) return 1;
            snprintf(msg, f_size+1, "[{%s}{%s}{%s}]", type, params, payload);
            free(type);
            free(params);
            free(payload);
        }
        if(NM_send_all(msg, strlen(msg), NO_TIMEOUT, ctx->connection) == -1) return 1;
    }



    return 0;
}