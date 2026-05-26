/*
    made by Ruben Amadei on xx/xx/xxxx

    utility functions
*/
#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

//debug list
#ifndef DEBUG_DO
    #define DEBUG_DO    0
#endif
#ifndef REQUEST_FORMAT_DEBUG    
    #define REQUEST_FORMAT_DEBUG    0
#endif
#ifndef CONFIG_DEBUG
    #define CONFIG_DEBUG    0
#endif
#ifndef REQUEST_DEBUG
    #define REQUEST_DEBUG    0
#endif
#ifndef COMMUNICATOR_DEBUG
    #define COMMUNICATOR_DEBUG    0
#endif

#define DEBUG(macro, statement) do{ \
        if(DEBUG_DO)        \
            if(macro)       \
                statement;   \
    }while(0);               \



void abort_error(char*);
void abort_numerror(char*, int);
void error(char*);
void numerror(char*, int);
void my_log(char*);
void my_log_f(char* fmt, ...);

char* input_string(int block_sz);

#endif