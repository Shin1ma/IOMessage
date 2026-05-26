/*
    made by Ruben Amadei on xx/xx/xxxx

    thread safe queue API for thread data exchange.
    Helper utilities for consumer-producer model O(1) insert pop
    blocking and non-blocking function (pop-wait/pop)
    error handler with values for OK, ERROR, WOULDBLOCK
*/

#ifndef SHARED_QUEUE_H
#define SHARED_QUEUE_H

#include "ThreadManager.h"

enum e_block_def{
    NONBLOCK = 0,
    BLOCK = 1,
};

typedef struct shared_queue {
    void** queue;    //buf
    void** queue_end; //end of buf
    void** tail; //data end
    void** head; //data start
    size_t size;    //number of elements in the queue
    size_t capacity; //max elements permitted
    TM_mutex_t queue_operation;
    TM_cond_var_t cond_var;
    int waiting; //number of threads waiting in cond
    int exit_state; //internal queue exit when destroying it
    int* external_exit_state;
    void (*destructor)(void*);
} shared_queue_t;

shared_queue_t* make_shared_queue(size_t cap, void (*destructor)(void*), int* external_exit_state);  //make the queue, destructor is optional, if NULL free will be used
size_t shared_queue_len(shared_queue_t* shq); //get the queue length
size_t shared_queue_cap(shared_queue_t* shq);   //get the queue capacity
//block is a boolean value that when set to 1 enables the queue to be blocking, if set to zero in cases it would block it returns NULL
int try_push_back(void* el, shared_queue_t* shq);  //push an element into the queue, ONLY HEAP ALLOCATED, queue gains ownership, exit is for global exit case, blocking
void* try_pop(shared_queue_t* shq); //pop an element from the queue, caller now has ownership of the element, blocking
int push_back(void* el, shared_queue_t* shq);  //push an element into the queue, ONLY HEAP ALLOCATED, queue gains ownership, non-blocking returns null if it would block
void* pop(shared_queue_t* shq); //pop an element from the queue, caller now has ownership of the element, non-blocking returns null if would block
void destroy_shared_queue(shared_queue_t* shq); //destroys the queue and the elements inside, can be blocking as it waits for remaining waiting threads to stop operating the queue. Guarantee that blocked threads will be let go, caller needs to guarantee that new concurrent operations have stopped before calling

#endif 
