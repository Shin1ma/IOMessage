/*
    Made by Ruben Amadei on the xx/xx/2026
*/

#include "../response.h"



response_t* make_resp(int status, const char* payload){
    assert(payload);
    
    response_t* res = malloc(sizeof(response_t));
    if(!res){
        error("couldn't allocate response type");
        return NULL;
    }
    
    size_t size = strlen(payload) + 1;
    char* payload_cpy = make_escaped_string(payload);
    if(!payload_cpy){
        free(res);
        error("couldn't escape payload");
        return NULL;
    }

    res->payload = payload_cpy;
    res->status = status;

    return res;
}
void destroy_resp(response_t* resp){
    assert(resp);
    free(resp->payload);
    free(resp);
}

char* make_raw_resp(const response_t* resp){
    assert(resp);

    int len;
    char* raw;   
    len = snprintf(NULL, 0, "[{%d}{%s}]", resp->status, resp->payload);
    raw = malloc(len+1);
    if(!raw){
        error("couldn't alloc raw response");
        return NULL;
    }
    snprintf(raw, len+1, "[{%d}{%s}]", resp->status, resp->payload);

    return raw;
}