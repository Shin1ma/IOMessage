/*
    made by Ruben amadei on the xx/xx/2026
*/

#include "../utils.h"

void abort_error(char* str){
    fprintf(stderr, "ERROR: %s, aborting...\n", str);
    exit(-1);
}
void abort_numerror(char* str, int n){
    fprintf(stderr, "ERROR: %s %d, aborting...\n", str, n);
    exit(n);
}
void error(char* str){
    fprintf(stderr, "ERROR: %s\n", str);
}
void numerror(char* str, int n){
    fprintf(stderr, "ERROR %s %d\n", str, n);
}
void my_log(char* str){
    printf("LOG: %s\n", str);
}
void my_log_f(char* fmt, ...){
    va_list argomenti;
    va_start(argomenti, fmt);

    char log_str[] = "LOG: ";
    size_t size = strlen(log_str)+strlen(fmt)+2;
    char* final_str = malloc(size);
    if(!final_str){
        error("error allocating string");
        return;
    }
    strncpy(final_str, log_str, size);
    strncat(final_str, fmt, size);
    final_str[size-2] = '\n';
    final_str[size-1] = '\0';

    vprintf(final_str, argomenti);

    va_end(argomenti);
}

char* input_string(int block_sz){
    int i = 0, j = 1;
    char* buf = (char*)calloc(block_sz, sizeof(char));
    if(!buf) return NULL;
    while((buf[i] = getchar()) != '\n'){
        if(i % (block_sz-1) == 0){
            buf = (char*)realloc(buf, block_sz*j);
            if(!buf) return NULL;
            j++;
        }
        i++;
    }
    buf[i] = '\0';
    return buf;
}
