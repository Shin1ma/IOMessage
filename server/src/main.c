/*
    made by Ruben Amadei on xx/xx/2026
*/


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "../utils.h"
#include "../NetworkManager.h"
#include "../ThreadManager.h"
#include "../CommunicationManager.h"
#include "../shared_queue.h"

#define WELL_KNOWN_PORTS    1024

/*
*    Main thread responsibilities:
*    - Accept incoming client connections
*    - Spawn server thread (TODO)
*    - Spawn communicator threads
*    - Maintain communicator lifecycle
*    - Maintain server lifecycle (TODO)
*
*    Communication flow:
*    Client -> CommunicationManager -> ServerManager -> CommunicationManagers -> Clients
*
*    Abstractions:
*    - NetworkManager: platform-specific networking
*    - ThreadManager: platform-specific threading and synchronization
*    - CommunicationManager: client communication logic
*    - ServerManager: manages server functions like: (TODO)
*       - sending messages
*       - secure sending
*       - login/register
*       - other...
*/



//utility functions
static void parse_args(int argc, const char** argv);

//communicator utility functions
//creates arguments for the communicator, owns the response queue and argument structure
static struct communicator_args* create_args_for_comm(struct communicator_args def, size_t communicator_id, NM_conn_t* conn);
static void destroy_args_for_comm(struct communicator_args* args);
//creates a communicator thread, owns the thread itself
static TM_thread_t* create_communicator(struct communicator_args def, size_t communicator_id, NM_conn_t* conn);
static void destroy_communicator(TM_thread_t* comm);

//utilities for thread list
static int is_comm_list_full(TM_thread_t** comm_array, size_t max); // 1 if full
static int search_free(TM_thread_t** comm_array, size_t max); //gets the first free index in the list
static void remove_stale_communicators(TM_thread_t** comm_array, size_t max); //destroys all exited communicators
static void wait_for_all_communicators(TM_thread_t** comm_array, size_t max); //joins all communicator threads

/*
    GLOB VARS
*/
int glob_exit = 0; //used to exit all threads (server and communicator)
static size_t param_port = 0; //provided server port

int main(int argc, char* argv[]){
    parse_args(argc, argv);

    //properties
    size_t max_clients = get_num_property("MAX_CLIENTS");
    
    char port_str[16];
    snprintf(port_str, 16, "%zu", param_port);
    
    NM_srv_context_t* network_context = NM_init_server(port_str);
    if(!network_context){
        abort_error("failed to start up networking");
    }
    
    arena_t* arena = create_arena();
    if(!arena){
        NM_srv_destroy(network_context);
        abort_error("failed to create mem pool");
    }

    TM_thread_t** communicator_list = arena_calloc(arena, max_clients, sizeof(TM_thread_t*));
    if(!communicator_list){
        NM_srv_destroy(network_context);
        abort_error("couldn't allocate space for communicator threads");
    }

    //shared server/communicator resources
    TM_mutex_t* production_mutex = arena_alloc(arena, sizeof(TM_mutex_t));
    TM_cond_var_t* production_cond_var = arena_alloc(arena, sizeof(TM_cond_var_t));
    shared_queue_t* request_queue = make_shared_queue(get_num_property("REQUEST_QUEUE_CAP"), destroy_req, &glob_exit); //request queue
    if(!request_queue || !production_mutex || !production_cond_var){
        NM_srv_destroy(network_context);
        arena_destroy(arena);
        if(request_queue) destroy_shared_queue(request_queue);
        abort_error("failed to allocate server resources");
    }

    struct communicator_args def_args = {
        .exit = &glob_exit,
        .request_queue = request_queue,
    };
    size_t communicator_id;
    TM_thread_t* new_communicator;
    NM_conn_t* new_connection;

    //accept loop
    while(!glob_exit){
        log_msg("awaiting connection on port %zu...", param_port);
        new_connection = NM_accept_conn(network_context);
        if(!new_connection){
            error("unable to accept connection");
            continue;
        }
        
        //check if we have space for a new client
        remove_stale_communicators(communicator_list, max_clients);
        if(is_comm_list_full(communicator_list, max_clients)){
            log_msg("capacity reached, connection refused");
            NM_destroy_conn(new_connection);    //we basically reject the request by closing the connection
            continue;
        }

        log_msg("accepted a connection");

        communicator_id = search_free(communicator_list, max_clients);
        new_communicator = create_communicator(def_args, communicator_id, new_connection);
        if(!new_communicator){
            error("failed to allocate thread to communicator, refusing connection");
            continue;
        }
        communicator_list[communicator_id] = new_communicator;
    }
    wait_for_all_communicators(communicator_list, max_clients);
    remove_stale_communicators(communicator_list, max_clients); //this should remove all the threads

    //destructors
    destroy_shared_queue(request_queue);
    arena_destroy(arena);
    NM_srv_destroy(network_context);

    return 0;
}

static void parse_args(int argc, const char** argv){
    int has_port = 0, has_conf = 0;
    
    if(argc < 2){
        error("usage: <server> -p <port> -c <config file>, default port and configs will be used");
    }

    int i;
    for(i=1; i < argc; i++){
        if(strcmp("-p", argv[i]) == 0){
            if(i+1 >= argc) abort_error("no port specified with -p specifier");
            errno = 0;
            param_port = strtol(argv[i+1], NULL, 10);
            if(param_port < 1024 || param_port > 65535 || errno){
                error("invalid/no port selected. Using default");
                param_port = 0;
                has_port = 0;
                continue;
            }
            i++;
            has_port = 1;
        }
        else if(strcmp("-c", argv[i]) == 0){
            if(i+1 >= argc) abort_error("no path specified with -c specifier");
            if(init_properties_by_file(argv[i+1])) {
                has_conf = 0;
                continue;
            }
            i++;
            has_conf = 1;
            log_msg("succesfully loaded property file");
            DEBUG(CONFIG_DEBUG, log_msg("conf file path: %s", argv[i+1]))
        }
    }

    if(has_port == 0) {
        log_msg("using default port");
        param_port = get_num_property("DEFAULT_PORT");
    }
    if(has_conf == 0) log_msg("using default config");
}

static struct communicator_args* create_args_for_comm(struct communicator_args def_args, size_t communicator_id, NM_conn_t* conn){
    shared_queue_t* response_queue = make_shared_queue(get_num_property("RESPONSE_QUEUE_CAP"), destroy_resp, &glob_exit);
    if(!response_queue){
        error("unable to allocate communicator response queue");
        return NULL;
    }
    struct communicator_args* args = malloc(sizeof(struct communicator_args));
    if(!args){
        destroy_shared_queue(response_queue);
        error("unable to allocate arguments for new communicator");
        return NULL;
    }
    
    memcpy(args, &def_args, sizeof(struct communicator_args));
    args->response_queue = response_queue;
    args->conn = conn;
    args->communicator_id = communicator_id;
    
    return args;
}
static void destroy_args_for_comm(struct communicator_args* args){
    destroy_shared_queue(args->response_queue);
    free(args);
}
static TM_thread_t* create_communicator(struct communicator_args def, size_t communicator_id, NM_conn_t* conn){
    struct communicator_args* comm_args = create_args_for_comm(def, communicator_id, conn);
    if(!comm_args) {
        NM_destroy_conn(conn);
        return NULL;
    }
    
    TM_thread_t* th = TM_create_thread(communicator_main, comm_args);
    if(!th){
        destroy_args_for_comm(comm_args);
        NM_destroy_conn(conn);
        return NULL;
    }
    return th;
}
static void destroy_communicator(TM_thread_t* comm){
    struct communicator_args* args = (struct communicator_args*)comm->args;
    destroy_args_for_comm(args);
    TM_destroy_thread(comm);
}

static int is_comm_list_full(TM_thread_t** comm_array, size_t max){
    int i;
    for(i=0; i<max; i++){
        if(!comm_array[i]) return 0;
    }
    return 1;
}
static int search_free(TM_thread_t** comm_array, size_t max){
    int i;
    for(i=0; i<max; i++){
        if(!comm_array[i]) return i;
    }
    return -1;
}
static void remove_stale_communicators(TM_thread_t** comm_array, size_t max){
    int i;
    for(i=0; i<max; i++){
        if(!comm_array[i]) continue;
        if(comm_array[i]->exited){
            destroy_communicator(comm_array[i]);
            comm_array[i] = NULL;
        }
    }
}
static void wait_for_all_communicators(TM_thread_t** comm_array, size_t max){
    int i;
    for(i=0; i<max; i++){
        if(comm_array[i]) TM_join(comm_array[i], TM_INFINITE, NULL);
    }
}