/*
*    made by Ruben Amadei on xx/xx/xxxx
*    
*    parses raw request into a structure
*
*   REQUEST FORMAT
*        responses follow this format: [{<type>}{<param1:value1>,<param2:value2>...}{<payload>}]
*       '[]' characters delimit the response start and end
*       '{}' characters delimit the beginning and end of a section
*       ':' characters separate name from value in parameters
*       ',' characters separate parameter from parameter
*       <type>: a request type, in string format that specifies the desired action, current types:
*           HEARTBEAT: signals that the client is still alive, ignored by the server
*                NO PARAMS
*           EXIT: signals that the client wants to exit, ignored by the server, makes the communicator service stop
*               NO PARAMS
*           LOGIN: let's the user login, (gives a key to encrypt and access to user file and channels)(TODO, NOT SURE)
*               PARAMS (TO DO)
*           SIGNIN: lets the user create an account (and user file) (TODO NOT SURE)
*               PARAMS (TO DO)
*           MSG: lets the user send a message (where he's authorized) (TODO NOT SURE)
*               PARAMS (TO DO)
*       <payload>: string containing body of the request, character escape
*       is achieved by using the '\' character before the symbol we want to escape, any escaped symbols will be ignored by
*       response parser (eg. \[ will be skipped entirely) and they will be treated as part of the payload and not the response eg. you can use {} in the payload by escaping them \{ \} or even \\
*
*
*    format [{type}{param1:value1,param2:value2...}{payload}]
*/

#ifndef REQUEST_H
#define REQUEST_H

#include <stdlib.h>
#include <string.h>

#include "../utils.h"

#define MAX_REQ_TYPE_LEN 20
#define MAX_REQ_TYPES 5

enum e_req_types {
    UNKNOWN = -1,
    HEARTBEAT = 0,
    EXIT = 1,
    LOGIN = 2,
    SIGNIN = 3,
    MSG_T = 4,
};

typedef struct req_types{
    char str_type[MAX_REQ_TYPE_LEN];
    enum e_req_types type;
} request_type_t;

extern const request_type_t types[MAX_REQ_TYPES];

typedef struct request_param {  //linked list of name-value params
    char* name;
    char* value;
    struct request_param* next;
    struct request_param* prev;
    arena_t* arena;
} req_param_t;

typedef struct request {
    size_t communicator_id;    
    enum e_req_types request_type;
    req_param_t* request_parameters;
    char* payload;
    char* raw_request;
    arena_t* arena;
} request_t;

//parameter utility
req_param_t* create_param_list(arena_t* arena, const char* name, const char* value);   //create the linked list
req_param_t* add_param(req_param_t* list, const char* name, const char* value);   //add a parameter to the linked list
req_param_t* search_param(req_param_t* list, const char* name); //search for a parameter by name

//utility
enum e_req_types string_type_to_int(const char* type);   //convert the request type to enum
void int_type_to_str(int type, char* buf, size_t buf_sz);

//structure formatting
request_t* make_req(const char* raw, size_t num); //make the request struct
void destroy_req(request_t* req);

#endif
