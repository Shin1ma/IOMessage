/*
    made by Ruben Amadei on xx/xx/xxxx

    main communicator thread, handles communication with client and ha a few request
    witch it can answer without giving the request to the ServerManager:
    hearthbeat -> ignored | if hearthbeat is not sent in 2 mins and no other request are received the CM will give a timedout response and exit
    exit -> gracefull connection close with exit response
*/
#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

#include "ThreadManager.h"
#include "NetworkManager.h"
#include "shared_queue.h"
#include "request.h"
#include "response.h"
#include "utils.h"

#define RECV_BLOCK_SZ   10

#define MAX_RECV_TIMEO  100
#define MAX_SND_TIMEO   2000 // 2 secs

enum communicator_local_exits{
    NO_EXIT = 0,
    GRACEFUL_EXIT_TRIGGERED = 1,
    UNGRACEFUL_EXIT_TRIGGERED = 2,
    GLOBAL_EXIT_TRIGGERED = 3,
};

struct communicator_args{
    NM_conn_t* conn;    //connection to client
    size_t communicator_id;   //used to identify the communicator

    //shared resources
    shared_queue_t* response_queue;
    shared_queue_t* request_queue;

    int* exit;  //used for global program exit
};

typedef struct recv_buffer{
    char* buf;
    int cap;
    int len;
} buf_t;

enum internal_status_codes{ //responses code for communicator only responses
    UNGRACEFUL_EXIT=100,
    GRACEFUL_EXIT=200
};

unsigned communicator_main(void* args); //main thread function
#endif