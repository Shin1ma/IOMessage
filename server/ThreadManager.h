/*
made by Ruben Amadei on xx/xx/xxxx

abstraction layer for OS thread creation and management
*/

#ifndef THREADMANAGER_H
#define THREADMANAGER_H
#ifdef _WIN32
#elif defined(__linux__)
#else
    #error "system not supported"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <process.h>
#elif defined(__linux__)

#endif

#include "utils.h"

#ifdef _WIN32
    typedef HANDLE TM_th_t;
    typedef DWORD TM_tid_t;
    typedef DWORD TM_ret_t;
    typedef CRITICAL_SECTION TM_mutex_t;
    typedef CONDITION_VARIABLE TM_cond_var_t;
    typedef HANDLE TM_sem_t;
#elif defined(__linux__)

#else
    #error "platform not supported"
#endif

enum TM_EnumTimeout{
    TM_INFINITE = -1
};
enum TM_CommErrors{
    TM_TIMEDOUT = -4,
    TM_ERR = -3,
    TM_ABANDON = -2
};

typedef unsigned (TM_thread_func_t)(void*);

typedef struct TM_thread{
    TM_th_t handle;
    int exited; //is the thread still running
    void* args;
    TM_thread_func_t* routine;
    #ifdef _WIN32
        TM_tid_t id;
    #endif
} TM_thread_t; // basic thread structure

TM_thread_t* TM_create_thread(TM_thread_func_t* routine, void* args); //create a thread with arg and routine
void TM_destroy_thread(TM_thread_t* th);

//mutex functions
void TM_mutex_create(TM_mutex_t* mtx);
void TM_mutex_lock(TM_mutex_t* mtx);
void TM_mutex_unlock(TM_mutex_t* mtx);
void TM_mutex_destroy(TM_mutex_t* mtx);

//condition vars functions
void TM_cond_create(TM_cond_var_t* cond);
int TM_cond_wait(TM_cond_var_t* cond, TM_mutex_t* mtx, int timeout);
void TM_cond_signal(TM_cond_var_t* cond);
void TM_cond_broadcast(TM_cond_var_t* cond);

//sem functions
TM_sem_t TM_make_sem(int count);
int TM_sem_wait(TM_sem_t sem, int timeout);
int TM_sem_signal(TM_sem_t sem);
void TM_destroy_sem(TM_sem_t sem);

//thread helpers
void TM_sleep(int ms);
void TM_yield();
int TM_join(const TM_thread_t* thread, int timeout, TM_ret_t* ret); //wait for a thread


#endif