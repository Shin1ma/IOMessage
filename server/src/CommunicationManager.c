/*
    made by Ruben amadei on the xx/xx/2026
*/

#include "../CommunicationManager.h"

static enum communicator_local_exits handle_client_errors(size_t client_errors, size_t client_timer, size_t communicator_id, size_t max_timeout, size_t max_errors, NM_conn_t* conn);
static int send_resp(int status, char* payload, size_t communicator_id, NM_conn_t* conn); //send a response to the client

static char* recv_req(size_t max_message_len, size_t communicator_id, buf_t* recv_buf, NM_conn_t* conn);   //receive a request from the client drop it if bad formatting
static buf_t* create_recv_buf(int cap);
static int buf_empty(buf_t* buf);
static int get_from_buf(buf_t* buf, char* str_buf, int len);
static char get_ch_from_buf(buf_t* buf);
static size_t put_in_buf(buf_t* buf, char* str_buf, int len);
static void destroy_buf(buf_t* buf);

static int send_all_resps(shared_queue_t* rsp_queue, size_t communicator_id, NM_conn_t* conn); //send all responses in the queue
static enum communicator_local_exits handle_request(request_t* request, size_t communicator_id, shared_queue_t* request_queue, NM_conn_t* conn);

static int* glob_exit;

unsigned communicator_main(void* args){
    struct communicator_args* comm_arg = args;
    NM_conn_t* conn = comm_arg->conn;
    shared_queue_t* response_queue = comm_arg->response_queue;
    shared_queue_t* request_queue = comm_arg->request_queue;
    size_t communicator_id = comm_arg->communicator_id;
    glob_exit = comm_arg->exit;

    enum communicator_local_exits loc_exit = NO_EXIT;

    size_t client_inactive_time = 0;  //timer for inactive clients
    size_t client_errors = 0; //errors clients produce
    int status;

    char* raw_request;
    request_t* request = NULL;

    size_t max_message_len = get_num_property("MAX_MESSAGE_LEN")*1024;
    size_t max_timeout = get_num_property("MAX_CLIENT_TIMEOUT")*1000;
    size_t max_errors = get_num_property("MAX_CLIENT_ERRORS");

    buf_t* recv_buf = create_recv_buf(RECV_BLOCK_SZ);
    if(!recv_buf){
        error("error allocating additional space for recv buffer in communicator %zu", communicator_id);
        loc_exit = UNGRACEFUL_EXIT;
    }
    while(!loc_exit && !*glob_exit){
        /*check for bad client, if the timeout time is maxed out
        send an ungraceful exit, or if errors are too many*/
        if((loc_exit = handle_client_errors(client_errors, client_inactive_time, communicator_id, max_timeout, max_errors, conn)) != NO_EXIT) continue;
        
        //check if we need to send any responses
        if((status = send_all_resps(response_queue, communicator_id, conn) != 0)){
            if(status == NM_CONN_CLOSED){
                log_msg("client with communicator id: %zu closed the connection unexpectedly", communicator_id);
                loc_exit = UNGRACEFUL_EXIT;
                continue;
            }
            DEBUG(COMMUNICATOR_DEBUG,error("not all responses in the queue where sent"))
        }
        raw_request = recv_req(max_message_len, communicator_id, recv_buf, conn);   //recv request
        /*
            if recv is unsuccesfull and connection is closed just exit. 
            If theres no need to exit just log the error, handle it and retry receiving.
        */
        if(!raw_request){
            status = NM_get_error(conn);
            switch (status)
            {
            case NM_CONN_CLOSED:
                loc_exit=UNGRACEFUL_EXIT_TRIGGERED;
                break;
            case NM_TIMEDOUT:
                client_inactive_time += MAX_RECV_TIMEO;
                break;
            default:
                client_errors++;
                break;
            }
            continue;
        }

        /*
             if a request is succesfully received reset the timer, format it and handle it
             Hearthbeat and exit requests are handled by the communicator and ignored by the server, other ones are sent to the server queue
        */
        client_inactive_time = 0; 

        request = make_req(raw_request, communicator_id);   //format the request into a structure
        free(raw_request);
        if(request == NULL){ 
            client_errors++;
            error("failed to format request");
            continue;
        }
        client_errors = 0; //reset errors upon succesfull request

        loc_exit = handle_request(request, communicator_id, request_queue, conn);
    }

    if(loc_exit == GLOBAL_EXIT_TRIGGERED){
        DEBUG(COMMUNICATOR_DEBUG, log_msg("Global exit for communicator %zu triggered", communicator_id))
        //TODO when i figure out the server
    }
    else if(loc_exit == UNGRACEFUL_EXIT_TRIGGERED){
        DEBUG(COMMUNICATOR_DEBUG, log_msg("Ungracefull exit for communicator %zu triggered", communicator_id))
        //TODO when i figure out the server
    } 
    else if(loc_exit == GRACEFUL_EXIT_TRIGGERED){
        DEBUG(COMMUNICATOR_DEBUG, log_msg("Gracefull exit for communicator %zu triggered", communicator_id))
        //TODO
    }
    log_msg("client %zu exited...", communicator_id, loc_exit);
    NM_destroy_conn(conn);  // close the connection
    return loc_exit;
}

static enum communicator_local_exits handle_client_errors(size_t client_errors, size_t client_timer, size_t communicator_id, size_t max_timeout, size_t max_errors, NM_conn_t* conn){
    if(client_timer >= max_timeout){
        DEBUG(COMMUNICATOR_DEBUG, log_msg("client %zu timed out", communicator_id));
        send_resp(UNGRACEFUL_EXIT, "your client timed-out or failed to send heartbeat", communicator_id, conn);
        return UNGRACEFUL_EXIT_TRIGGERED;
    }
    if(client_errors >= max_errors){   //we dont want bad clients
        DEBUG(COMMUNICATOR_DEBUG, log_msg("client %zu errored out", communicator_id));
        send_resp(UNGRACEFUL_EXIT, "your client generates too many errors/the server is toast", communicator_id, conn);
        return UNGRACEFUL_EXIT_TRIGGERED;
    }
    return NO_EXIT;
}
static enum communicator_local_exits handle_request(request_t* request, size_t communicator_id, shared_queue_t* request_queue, NM_conn_t* conn){
    int status;
    IF_DEBUG_START(REQUEST_DEBUG)
        char str_type[20];
        req_param_t* p;
        int_type_to_str(request->request_type, str_type, 20);
        log_msg("\n\nclient %zu received request\n req type: %s,\n req_params:", communicator_id, str_type);
        for(p = request->request_parameters; p != NULL; p = p->next){
            log_msg("\t name:%s\t\\\tvalue:%s", p->name, p->value);
        }
        log_msg("payload: %s\n\n", request->payload);
    END_IF_DEBUG()

    switch (request->request_type){
        case HEARTBEAT:{ //just hb throw away the request
            DEBUG(REQUEST_DEBUG, log_msg("client %zu received Hearthbeat", communicator_id));
            destroy_req(request);
            break;
        }
        case EXIT:{  //client wants to exit
            DEBUG(REQUEST_DEBUG, log_msg("client %zu received exit", communicator_id));
            destroy_req(request);
            send_resp(GRACEFUL_EXIT, "exited succesfully", communicator_id, conn);
            return GRACEFUL_EXIT_TRIGGERED;
        }
        default:{    //server-side request
            status = push_back(request, request_queue);
            if(status){
                error("failed insert in request queue");
                destroy_req(request);
                if(*glob_exit) return GLOBAL_EXIT_TRIGGERED;
                return NO_EXIT;
            }
            DEBUG(REQUEST_DEBUG, log_msg("request succesfully sent to the server from communicator number %zu", communicator_id));
            break;
        }
    }
    return NO_EXIT;
}

static int send_resp(int status, char* payload, size_t communicator_id, NM_conn_t* conn){
    char* raw;
    response_t response = {
        .payload = payload,
        .status = status
    };

    raw = make_raw_resp(&response);
    if(!raw){
        error("couldn't make a raw response client, communicator id: %zu", communicator_id);
        return -1;
    }

    int err = NM_send_all(raw, strlen(raw), MAX_SND_TIMEO, conn);
    free(raw);
    return err;
}
static int send_all_resps(shared_queue_t* response_queue, size_t communicator_id, NM_conn_t* conn){
    response_t* resp;
    int ret = 0;

    
    while((resp = try_pop(response_queue)) != NULL){
        if(send_resp(resp->status, resp->payload, communicator_id, conn) == -1){
            if(NM_get_error(conn) == NM_CONN_CLOSED) {
                destroy_resp(resp);
                return NM_CONN_CLOSED;
            }
            ret = -1;
            DEBUG(DEBUG_DO,error("couldn't correctly send a response, communicator id: %zu", communicator_id))
        }
        destroy_resp(resp);
    }
    return ret;
}


static char* recv_req(size_t max_msg_len, size_t communicator_id, buf_t* recv_buf, NM_conn_t* conn){
    int received;
    size_t len = 0;
    size_t cap = RECV_BLOCK_SZ + 1;
    char tmp_buf[RECV_BLOCK_SZ+1];
    char* req_cpy;
    char* req_start = malloc(RECV_BLOCK_SZ+1);
    if(!req_start){
        error("unable to allocate request body, communicator id: %zu", communicator_id);
        return NULL;
    }
    char* cursor = req_start;
    char* req_end;
    while(1){
        //get a block at a time until you get the request end
        if(!buf_empty(recv_buf)) received = get_from_buf(recv_buf, tmp_buf, RECV_BLOCK_SZ);
        else received = NM_recv(tmp_buf, RECV_BLOCK_SZ, MAX_RECV_TIMEO, conn);
        if(received <= 0){
            free(req_start);
            return NULL;
        }
        if(len+received >= max_msg_len){
            DEBUG(DEBUG_DO,error("max request size received, communicator id: %zu", communicator_id))
            free(req_start);
            return NULL;
        }
        tmp_buf[received] = '\0';
        
        if(len+received >= cap){    //reallocate if string goes over limit
            while(len+received>=cap)cap *= 2;
            req_cpy = realloc(req_start, cap);
            if(!req_cpy){
                error("unable to reallocate request to accomodate new data, communicator id: %zu", communicator_id);
                free(req_start);
                return NULL;
            }
            req_start = req_cpy;
        }

        cursor = &req_start[len];
        copy_str(cursor, tmp_buf, received+1);
        len+=received;
        if((req_end=escaped_strchr(req_start, ']')) != NULL){  //end of request
            if(*(req_end+1) != '\0'){
                int remaining = (int)strlen(req_end+1);
                put_in_buf(recv_buf, req_end+1, remaining);
                *(req_end+1) = '\0';
            }
            DEBUG(REQUEST_FORMAT_DEBUG, log_msg("req: %s", req_start))
            return req_start;
        }
    }
}
static buf_t* create_recv_buf(int cap){
    assert(cap > 0);
    buf_t* new_buf = malloc(sizeof(buf_t));
    if(!new_buf) return NULL;
    new_buf->len = 0;
    new_buf->cap = cap+1;
    new_buf->buf = malloc(cap);
    if(!new_buf->buf){
        free(new_buf);
        return NULL;
    }
    return new_buf;
}
static int buf_empty(buf_t* buf){
    return buf->len <= 0;
}
static char get_ch_from_buf(buf_t* buf){
    if(buf->len <= 0) return -1;
    char ch = buf->buf[0];
    int i;

    for(i = 1; i < buf->len; i++){
        buf->buf[i-1] = buf->buf[i];
    }
    buf->len--;
    return ch;
}
static int get_from_buf(buf_t* buf, char* str_buf, int len){
    if(len > buf->len) len = buf->len;
    int i;
    for(i=0; i < len; i++){
        str_buf[i] = get_ch_from_buf(buf);
    }
    return len;
}
static size_t put_in_buf(buf_t* buf, char* str_buf, int len){
    if(len > buf->cap - buf->len) len = buf->cap - buf->len;
    char* free_buf = &buf->buf[buf->len];
    int i;
    for(i=0; i<len; i++){
        free_buf[i] = str_buf[i];
    }
    buf->len += len;
    return len;
}
static void destroy_buf(buf_t* buf){
    free(buf->buf);
    free(buf);
}
