#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // PSEUDOCODE:
    //   Cast thread_param into a struct thread_data pointer.
    //   Sleep for wait_to_obtain_ms to simulate pre-lock work.
    //   Lock the mutex and bail out on error.
    //   Sleep for wait_to_release_ms while holding the mutex.
    //   Unlock the mutex and mark the thread as successful.
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    thread_func_args->thread_complete_success = false;

    if (usleep((useconds_t)thread_func_args->wait_to_obtain_ms * 1000U) != 0) {
        ERROR_LOG("Failed to sleep before obtaining mutex");
    }

    if (pthread_mutex_lock(thread_func_args->mutex) != 0) {
        ERROR_LOG("Failed to lock mutex");
        return thread_param;
    }

    if (usleep((useconds_t)thread_func_args->wait_to_release_ms * 1000U) != 0) {
        ERROR_LOG("Failed to sleep before releasing mutex");
    }

    if (pthread_mutex_unlock(thread_func_args->mutex) != 0) {
        ERROR_LOG("Failed to unlock mutex");
        return thread_param;
    }

    thread_func_args->thread_complete_success = true;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * PSEUDOCODE:
     *   Allocate thread_data on the heap to survive past this call.
     *   Populate the wait timings and mutex pointer.
     *   Set thread_complete_success false until the thread finishes.
     *   Create the thread with threadfunc as the entry point.
     *   Free memory and return false if creation fails.
     *   Return true on success without waiting for completion.
     */
    struct thread_data *thread_data = malloc(sizeof(*thread_data));
    if (thread_data == NULL) {
        ERROR_LOG("Failed to allocate thread data");
        return false;
    }

    thread_data->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_data->wait_to_release_ms = wait_to_release_ms;
    thread_data->mutex = mutex;
    thread_data->thread_complete_success = false;

    if (pthread_create(thread, NULL, threadfunc, thread_data) != 0) {
        ERROR_LOG("Failed to create thread");
        free(thread_data);
        return false;
    }

    return true;
}
