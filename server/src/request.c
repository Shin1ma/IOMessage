/*
    made by Ruben amadei on the xx/xx/2026
*/

#include "../request.h"

const request_type_t types[MAX_REQ_TYPES] = {
    {
        .str_type = "HEARTBEAT",
        .type = HEARTBEAT
    },
    {
        .str_type = "EXIT",
        .type = EXIT
    },
    {
        .str_type = "LOGIN",
        .type = LOGIN
    },
    {
        .str_type = "SIGNIN",
        .type = SIGNIN
    },
    {
        .str_type = "MSG_T",
        .type = MSG_T
    }
};

static req_param_t* create_node(arena_t* arena, const char* name, const char* value){
    assert(arena && name && value);
    
    size_t name_len = strlen(name) + 1;
    size_t value_len = strlen(value) + 1;

    arena_t* tmp_arena = create_arena();
    if(!tmp_arena){
        error("failed to allocate arena pool for params");
        return NULL;
    }

    struct request_param* new_node = arena_alloc(tmp_arena, sizeof(struct request_param));
    if(!new_node){
        arena_destroy(tmp_arena);
        error("failed to create new param node");
        return NULL;
    }
    new_node->next = NULL;
    new_node->arena = arena;

    new_node->name = arena_alloc(tmp_arena, name_len);
    if(!new_node->name){
        error("unable to allocate name to param node");
        arena_destroy(tmp_arena);
        return NULL;
    }

    new_node->value = arena_alloc(tmp_arena, value_len);
    if(!new_node->value){
        error("unable to allocate value to param node");
        arena_destroy(tmp_arena);
        return NULL;
    }

    arena_give_child(arena, tmp_arena);

    copy_str(new_node->name, name, name_len);
    copy_str(new_node->value, value, value_len);

    return new_node;
}
req_param_t* create_param_list(arena_t* arena, const char* name, const char* value){
    assert(arena && name && value); 
    
    req_param_t* new_node = create_node(arena, name, value);
     if(!new_node){
        error("unable to create param list");
        return NULL;
    }
    new_node->arena = arena;
    new_node->prev = new_node;
    return new_node;
}
req_param_t* add_param(struct request_param* list, const char* name, const char* value){
    assert(list && name && value); 

    struct request_param* end = list->prev;
    req_param_t* new_node = create_node(list->arena, name, value);
    if(!new_node){
        error("unable to add node to list");
        return NULL;
    }

    list->prev = new_node;
    end->next = new_node;
    new_node->prev = end;
    return new_node;
}
req_param_t* search_param(struct request_param* list, const char* name){
    assert(list && name); 
    
    struct request_param* node;
    for(node = list; node; node = node->next){
        if(strcmp(node->name, name) == 0) return node;
    }
    return NULL;
}

enum e_req_types string_type_to_int(const char* type){
    assert(type);

    int i;
    for(i=0; i < MAX_REQ_TYPES; i++){
        if(strcmp(types[i].str_type, type) == 0) return types[i].type;
    }
    return UNKNOWN;
}
void int_type_to_str(int type, char* buf, size_t buf_sz){
    if(type >= 0 && type < MAX_REQ_TYPES){
        copy_str(buf, types[type].str_type, buf_sz);
        return;
    }
    copy_str(buf, "UNKNOWN", buf_sz);
    return;
}

static request_t* allocate_request(const char* raw){
    assert(raw);

    size_t raw_buf_len = strlen(raw)+1;

    arena_t* request_arena = create_arena();
    if(!request_arena){
        error("failed to allocate arena pool for request");
        return NULL;
    }
    request_t* req = arena_alloc(request_arena, sizeof(request_t));
    if(!req){
        arena_destroy(request_arena);
        error("failed to allocate request struct");
        return NULL;
    }
    req->arena = request_arena;
    req->raw_request = arena_alloc(request_arena, raw_buf_len);
    if(!req->raw_request){
        destroy_req(req);
        error("failed to allocate raw str for request struct");
        return NULL;
    }

    copy_str(req->raw_request, raw, raw_buf_len);
    return req;
}
static char* get_type(arena_t* arena, char** cursor){
    assert(cursor && arena);
    char* type_start = escaped_strchr(*cursor, '{');
    char* type_end = escaped_strchr(*cursor, '}');
    if(!type_start){
        DEBUG(REQUEST_FORMAT_DEBUG,error("invalid request formatting, missing type start delimiter"))
        return NULL;
    }
    if(!type_end){
        DEBUG(REQUEST_FORMAT_DEBUG,error("invalid request formatting, missing type end delimiter"))
        return NULL;
    }
    type_start++;

    size_t type_size = (size_t)(type_end - type_start);
    char* type_str = arena_alloc(arena, type_size+1);
    if(!type_str) return NULL;

    get_substr(type_str, type_start, type_end, type_size+1);

    *cursor = type_end+1;
    return type_str;
}
static char* get_params(arena_t* arena, const char** cursor){
    assert(cursor && arena);
    char* param_start = escaped_strchr(*cursor, '{');
    char* param_end = escaped_strchr(*cursor, '}');
    if(!param_start){
        DEBUG(REQUEST_FORMAT_DEBUG,error("invalid request formatting, missing param start delimiter"))
        return NULL;
    }
    if(!param_end){
        DEBUG(REQUEST_FORMAT_DEBUG,error("invalid request formatting, missing param end delimiter"))
        return NULL;
    }
    param_start++;

    size_t param_size = (size_t)(param_end - param_start);
    char* param_str = arena_alloc(arena, param_size+1);
    if(!param_str) return NULL;
    
    get_substr(param_str, param_start, param_end, param_size+1);

    *cursor = param_end+1;
    return param_str;
}
static char* get_payload(arena_t* arena, const char** cursor){
    assert(cursor && arena);
    char* payload_start = escaped_strchr(*cursor, '{');
    char* payload_end = escaped_strchr(*cursor, '}');
    if(!payload_start){
        DEBUG(REQUEST_FORMAT_DEBUG,error("invalid request formatting, missing payload start delimiter"))
        return NULL;
    }
    if(!payload_end){
        DEBUG(REQUEST_FORMAT_DEBUG,error("invalid request formatting, missing payload end delimiter"))
        return NULL;
    }
    payload_start++;

    size_t payload_size = (size_t)(payload_end - payload_start);
    char* payload_str = arena_alloc(arena, payload_size+1);
    if(!payload_str) return NULL;

    get_substr(payload_str, payload_start, payload_end, payload_size+1);
    
    *cursor=payload_end+1;
    return payload_str;
}
static req_param_t* format_params(arena_t* arena, const char* param_str){
    assert(param_str && arena);

    size_t param_str_len = strlen(param_str);

    char* param_str_cpy = malloc(param_str_len+1);
    if(!param_str_cpy){
        error("couldn't allocate for param string copy");
        return NULL;
    }
    copy_str(param_str_cpy, param_str, param_str_len+1);

    char* name;
    char* value;
    char* next_pair = param_str_cpy;
    char* curr_pair;
    char* name_delimiter;
    char* pair_delimiter;
    req_param_t* new_params = NULL;
    req_param_t* node;
    while(next_pair){    //set up parameter list
        curr_pair = next_pair;

        pair_delimiter = escaped_strchr(curr_pair, ',');
        //if there isnt a next pair, next pair will be left null and loop will exit
        next_pair = NULL;
        if(pair_delimiter != NULL){
            *pair_delimiter = '\0'; //get a curr_pair substring
            next_pair = pair_delimiter+1;
        }

        name = curr_pair;
        name_delimiter = escaped_strchr(curr_pair, ':');
        if(name_delimiter == NULL || name_delimiter == name){   //name doesnt exists
            DEBUG(REQUEST_FORMAT_DEBUG,error("invalid parameter name:value pair, skipping"))
            continue;
        }
        else{
            *name_delimiter = '\0'; //get name substring
            value = name_delimiter+1;   //get value substring
        }

        if(new_params == NULL) {
            new_params = create_param_list(arena, name, value); //if the list isn't yet created initialize it
            node = new_params;
        }
        else{
            node = add_param(new_params, name, value);
        }
        if(!node) error("something went wrong, skipping param");
    }
    free(param_str_cpy);
    return new_params;
}
static int format_req(const char* raw, request_t* req){
    assert(raw && req);

    size_t raw_str_len = strlen(raw);

    if(raw_str_len < 8){    //minimum for control characters
        DEBUG(REQUEST_FORMAT_DEBUG,error("invalid req format (request too short)"))
        return 1;
    }
    if(raw[0] != '[' || raw[raw_str_len-1] != ']'){   //check for basic delimiters
        DEBUG(REQUEST_FORMAT_DEBUG,error("invalid req format (missing start/end delimiter or type/payload delimiter)"))
        return 1;
    }

    arena_t* func_arena = create_arena();
    if(!func_arena){
        error("couldn't allocate arena pool");
        return 1;
    }
    const char* cursor = raw;
    char* type = get_type(func_arena, &cursor);
    char* param_str = get_params(func_arena, &cursor);
    char* payload = get_payload(func_arena, &cursor);
    if(!type || !param_str || !payload){
        arena_destroy(func_arena);
        DEBUG(REQUEST_FORMAT_DEBUG,error("invalid req format, skipping"))
        return 1;
    }
    if(*type == '\0'){
        arena_destroy(func_arena);
        DEBUG(REQUEST_FORMAT_DEBUG,error("request must have a type"))
        return 1;
    }

    size_t payload_len = strlen(payload);
    req->payload = arena_alloc(req->arena, payload_len+1);
    if(!req->payload){
        arena_destroy(func_arena);
        error("error allocating payload for request");
        return 1;
    }

    req->request_type = string_type_to_int(type);
    req->request_parameters = format_params(req->arena, param_str);
    if(!req->request_parameters){
        DEBUG(REQUEST_FORMAT_DEBUG,error("no params identified or error in alloc"))
    }
    copy_str(req->payload, payload, payload_len+1);

    arena_destroy(func_arena);
    return 0;
}
request_t* make_req(const char* raw, size_t communicator_id){
    assert(raw);

    request_t* req = allocate_request(raw);
    if(!req){
        error("error allocating request, skipping");
        return NULL;
    }
    if(format_req(raw, req)){
        destroy_req(req);
        DEBUG(REQUEST_FORMAT_DEBUG,log_msg("invalid req format"))
        return NULL;
    }
    req->communicator_id = communicator_id; //communicator that generated the request
    return req;
}

void destroy_req(request_t* req){
    assert(req);
    arena_destroy(req->arena);
}
