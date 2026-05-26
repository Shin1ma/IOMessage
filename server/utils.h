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
#include <assert.h>
#include "arena.h"

#define MAX_NAME    100
#define MAX_VAL     100
#define ESCAPED_CHARS "\\{}[]:,"

//debug utils

//debug var list
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

//debug statement
#define DEBUG(macro, statement) do{ \
            if(macro)       \
                statement;   \
    }while(0);               \


//debug code piece
#define IF_DEBUG_START(macro) if(macro){
    
#define END_IF_DEBUG() }

//no debug
#ifdef NDEBUG
#define DEBUG(macro, statement) do{}while(0);
#define IF_DEBUG_START(macro) if(0){
#endif

//logging utils
void abort_error(char* fmt, ...);
void error(char* fmt, ...);
void log_msg(char* fmt, ...);

//string utils
size_t copy_str(char* dst, const char* src, size_t len);
size_t get_substr(char* dst, const char* src_start, const char* src_end, size_t len);
char* src_and_replace(const char* needle, const char* substr, const char* src_str);
int is_escaped(const char* ch); //bool, checks if a character is escaped (1) or not (0)
char* escaped_strchr(const char* str, char ch); //like strchr but skips escaped strings
char* make_escaped_string(const char* orig_string);

//property file utils
typedef struct properties{
    char name[MAX_NAME];
    char val[MAX_VAL];
} property_t;

#define N_PROPERTIES    7
extern property_t glob_property_list_DEF[N_PROPERTIES];
extern property_t glob_property_list[N_PROPERTIES];

property_t* get_property_by_name(const char* name);
size_t get_num_property(const char* name);
void get_str_property(const char* name, char* dest, int dest_size);

int init_properties_by_file(const char* config_path);
size_t get_line_size(FILE* fp);


#endif