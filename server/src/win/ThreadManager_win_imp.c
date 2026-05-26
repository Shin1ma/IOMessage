/*
    made by Ruben amadei on the xx/xx/2026
*/

#include "../../ThreadManager.h"

struct TM_win_cb_wrap{
    TM_thread_func_t* func;
    int* exited;
    void* arg;
};

unsigned __stdcall TM_thread_routine_init(void* args){  //wrapping because windoes needs __stdcall
    struct TM_win_cb_wrap* wrapper = (struct TM_win_cb_wrap*)args;
    unsigned ret = wrapper->func(wrapper->arg);
    *wrapper->exited = 1;
    free(wrapper);
    return ret;
}

TM_thread_t* TM_create_thread(TM_thread_func_t* routine, void* args){
    TM_thread_t* new_thread = malloc(sizeof(TM_thread_t));
    if(!new_thread){
        error("failed to allocate new thread");
        return NULL;
    }
    struct TM_win_cb_wrap* wrapper = malloc(sizeof(struct TM_win_cb_wrap));
    if(!wrapper){
        free(new_thread);
        error("failed to allocate new wrapper");
        return NULL;
    }

    wrapper->arg = args;
    wrapper->func = routine;
    wrapper->exited = &new_thread->exited;
    
    new_thread->args = args;
    new_thread->routine = routine;
    new_thread->exited = 0;
    new_thread->handle = (HANDLE)_beginthreadex(NULL, 0, TM_thread_routine_init, wrapper, 0, &new_thread->id);
    if(!new_thread->handle){
        error("couldn't create thread");
        free(wrapper);
        free(new_thread);
        return NULL;
    }
    return new_thread;
}

int TM_join(const TM_thread_t* thread, int timeout, TM_ret_t* ret){
    if(timeout == TM_INFINITE) timeout = INFINITE;

    int status = WaitForSingleObject(thread->handle, timeout);
    if(status == WAIT_TIMEOUT) return TM_TIMEDOUT;
    if(status == WAIT_FAILED || status == WAIT_ABANDONED) return TM_ERR;

    if(ret != NULL){
    if(!GetExitCodeThread(thread->handle, ret)) error("error getting return of thread");
    }
    return 0;
}

void TM_mutex_create(TM_mutex_t* mtx){
    InitializeCriticalSection(mtx);
}
void TM_mutex_lock(TM_mutex_t* mtx){
    EnterCriticalSection(mtx);
}
void TM_mutex_unlock(TM_mutex_t* mtx){
    LeaveCriticalSection(mtx);
}
void TM_mutex_destroy(TM_mutex_t* mtx){
    DeleteCriticalSection(mtx);
}

void TM_cond_create(TM_cond_var_t* cond){
    InitializeConditionVariable(cond);
}
int TM_cond_wait(TM_cond_var_t* cond, TM_mutex_t* mtx, int timeout){
    if(timeout == TM_INFINITE) timeout = INFINITE;
    int status;

    if(!(status = SleepConditionVariableCS(cond, mtx, timeout))){
        status = GetLastError();
        switch(status){
            case ERROR_TIMEOUT:
                return TM_TIMEDOUT;
            default:
                return TM_ERR;
        }
    }
    return 0;
}
void TM_cond_signal(TM_cond_var_t* cond){
    WakeConditionVariable(cond);
}
void TM_cond_broadcast(TM_cond_var_t* cond){
    WakeAllConditionVariable(cond);
}

TM_sem_t TM_make_sem(int count){
    HANDLE sem = CreateSemaphore(NULL, count, LONG_MAX, NULL);
    if(sem == NULL){
        error("initialization of semaphore failed...");
        return NULL;
    }
    return sem;
}
int TM_sem_wait(TM_sem_t sem, int timeout){
    if(timeout==TM_INFINITE) timeout = INFINITE;
    int status = WaitForSingleObject(sem, timeout);
    switch(status){
        case WAIT_OBJECT_0: return 0;
        case WAIT_TIMEOUT: return TM_TIMEDOUT;
        default: return TM_ERR;
    }
}
int TM_sem_signal(TM_sem_t sem){
    return ReleaseSemaphore(sem, 1, NULL) == 0 ? TM_ERR : 0;
}
void TM_destroy_sem(TM_sem_t sem){
    CloseHandle(sem);
}

void TM_yield(){
    SwitchToThread();
}

void TM_sleep(int ms){
    Sleep(ms);
}

void TM_destroy_thread(TM_thread_t* th){
    CloseHandle(th->handle);
    free(th);
}

