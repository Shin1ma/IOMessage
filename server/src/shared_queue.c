/*
    made by Ruben amadei on the xx/xx/2026
*/

#include "../shared_queue.h"
#include "../utils.h"

shared_queue_t* make_shared_queue(size_t cap, void (*destructor)(void*), int* external_exit_state){
    assert(external_exit_state);
    
    if(cap == 0) {
        return NULL;
    }
    
    shared_queue_t* shq = malloc(sizeof(shared_queue_t));
    if(!shq){
        error("error allocating shared queue");
        return NULL;
    }
    shq->size = 0;
    shq->waiting = 0;
    shq->exit_state = 0;
    shq->destructor = destructor;
    shq->external_exit_state = external_exit_state;
    shq->capacity = cap;
    shq->queue = calloc(cap, sizeof(void*));
    if(!shq->queue){
        free(shq);
        error("error allocating queue");
        return NULL;
    }
    shq->queue_end = shq->queue+shq->capacity;
    shq->head = shq->queue;
    shq->tail = shq->queue;
    
    TM_mutex_create(&shq->queue_operation);
    TM_cond_create(&shq->cond_var);
    return shq;
}
size_t shared_queue_len(shared_queue_t* shq){
    assert(shq);
    
    size_t var;
    TM_mutex_lock(&shq->queue_operation);
    var = shq->size;
    TM_mutex_unlock(&shq->queue_operation);
    return var;
}
size_t shared_queue_cap(shared_queue_t* shq){
    assert(shq);
    
    size_t var;
    TM_mutex_lock(&shq->queue_operation);
    var = shq->capacity;
    TM_mutex_unlock(&shq->queue_operation);
    return var;
}

void push_back_el(void* el, shared_queue_t* shq){
    assert(shq && el && shq->size < shq->capacity);
    *shq->tail = el;
    shq->tail++;
    if(shq->tail == shq->queue_end) shq->tail = shq->queue;
    shq->size++;
}
void* pop_el(shared_queue_t* shq){
    assert(shq && shq->size > 0);
    void* el = *shq->head;
    *shq->head = NULL;
    shq->head++;
    if(shq->head == shq->queue_end) shq->head = shq->queue;
    shq->size--;
    return el;
}

int push_back(void* el, shared_queue_t* shq){
    assert(el && shq);

    TM_mutex_lock(&shq->queue_operation);
    shq->waiting++;
    while(shq->size >= shq->capacity && !shq->exit_state && !*shq->external_exit_state){
        TM_cond_wait(&shq->cond_var, &shq->queue_operation, TM_INFINITE);
    }
    if(shq->exit_state || *shq->external_exit_state) {
        shq->waiting--;
        TM_cond_broadcast(&shq->cond_var);
        TM_mutex_unlock(&shq->queue_operation);
        return 1;
    }
    shq->waiting--;

    push_back_el(el, shq);
    TM_cond_broadcast(&shq->cond_var);
    TM_mutex_unlock(&shq->queue_operation);
    return 0;
}
int try_push_back(void* el, shared_queue_t* shq){
    assert(el && shq);

    TM_mutex_lock(&shq->queue_operation);
    if(shq->size >= shq->capacity){
        TM_mutex_unlock(&shq->queue_operation);
        return 1;
    }
    push_back_el(el, shq);
    TM_cond_broadcast(&shq->cond_var);
    TM_mutex_unlock(&shq->queue_operation);
    return 0;
}
void* pop(shared_queue_t* shq){
    assert(shq);
    TM_mutex_lock(&shq->queue_operation);
    shq->waiting++;
    while(shq->size == 0 && !shq->exit_state && !*shq->external_exit_state){
        TM_cond_wait(&shq->cond_var, &shq->queue_operation, TM_INFINITE);
    }
    if(shq->exit_state || *shq->external_exit_state) {
        shq->waiting--;
        TM_cond_broadcast(&shq->cond_var);
        TM_mutex_unlock(&shq->queue_operation);
        return NULL;
    }
    shq->waiting--;
    
    void* popped = pop_el(shq);
    TM_cond_broadcast(&shq->cond_var);
    TM_mutex_unlock(&shq->queue_operation);
    return popped;
}
void* try_pop(shared_queue_t* shq){
    assert(shq);
    TM_mutex_lock(&shq->queue_operation);
    if(shq->size == 0){
        TM_mutex_unlock(&shq->queue_operation);
        return NULL;
    }
    void* popped = pop_el(shq);
    TM_cond_broadcast(&shq->cond_var);
    TM_mutex_unlock(&shq->queue_operation);
    return popped;
}
void destroy_shared_queue(shared_queue_t* shq){
    void* el;
    TM_mutex_lock(&shq->queue_operation);
    shq->exit_state = 1;
    while(shq->waiting){
        TM_cond_broadcast(&shq->cond_var);
        TM_cond_wait(&shq->cond_var, &shq->queue_operation, TM_INFINITE);
    }
    TM_mutex_unlock(&shq->queue_operation);
    while((el = pop(shq)) != NULL) shq->destructor ? shq->destructor(el) : free(el);
    TM_mutex_destroy(&shq->queue_operation);
    free(shq->queue);
    free(shq);
}