
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>

void* Malloc(size_t size)
{
    void* result = malloc(size);
    assert(
        result != NULL
    );

    return result;
}

void Pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr)
{
    assert(
        pthread_mutex_init(mutex, mutexattr) == 0
    );
}

void Pthread_mutex_lock(pthread_mutex_t* mutex)
{
    assert(
        pthread_mutex_lock(mutex) == 0
    );
}

void Pthread_mutex_unlock(pthread_mutex_t* mutex)
{
    assert(
        pthread_mutex_unlock(mutex) == 0
    );
}

void Pthread_create(pthread_t* thread,
    const pthread_attr_t* attr,
    void* (*start_routine) (void*),
    void* arg)
{
    assert(
        pthread_create(thread, attr, start_routine, arg) == 0
    );
}

void Pthread_join(pthread_t thread, void** retval)
{
    assert(
        pthread_join(thread, retval) == 0
    );
}

pthread_t Pthread_self()
{
    pthread_t result = pthread_self();
    assert(
        result != 0
    );
    return result;
}