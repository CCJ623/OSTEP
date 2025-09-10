#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "common_threads.h"

//
// Here, you have to write (almost) ALL the code. Oh no!
// How can you show that a thread does not starve
// when attempting to acquire this mutex you build?
//

typedef struct __ns_mutex_t
{
    sem_t lock_;
    sem_t queue_lock_;
    size_t serving_id_;
    size_t last_id_;
} ns_mutex_t;

ns_mutex_t mutex;

void ns_mutex_init(ns_mutex_t* m)
{
    Sem_init(&(m->lock_), 1);
    Sem_init(&(m->queue_lock_), 1);
    m->serving_id_ = m->last_id_ = 0;
}

void ns_mutex_acquire(ns_mutex_t* m)
{
    Sem_wait(&(m->queue_lock_));
    size_t my_id = m->last_id_;
    ++(m->last_id_);
    Sem_post(&(m->queue_lock_));

    Sem_wait(&(m->lock_));
    while (m->serving_id_ != my_id)
    {
        printf("waiting\n");
        Sem_post(&(m->lock_));
        Sem_wait(&(m->lock_));
    }
}

void ns_mutex_release(ns_mutex_t* m)
{
    ++(m->serving_id_);
    Sem_post(&(m->lock_));
}


void* worker(void* arg)
{
    int thread_id = *(int*)arg;
    srand(thread_id);
    sleep(rand() % 4);
    printf("worker %d enter\n", thread_id);
    ns_mutex_acquire(&mutex);
    printf("worker %d is working\n", thread_id);
    sleep(rand() % 3);
    ns_mutex_release(&mutex);
    printf("worker %d done\n", thread_id);
    return NULL;
}

// arg : number of threads

int main(int argc, char* argv[])
{
    assert(argc == 2);
    int thread_num = atoi(argv[1]);
    pthread_t threads[thread_num];
    int thread_ids[thread_num];
    ns_mutex_init(&mutex);

    printf("parent: begin\n");
    for (size_t i = 0; i != thread_num; ++i)
    {
        thread_ids[i] = i;
        Pthread_create(&threads[i], NULL, worker, &thread_ids[i]);
    }
    for (size_t i = 0; i != thread_num; ++i)
    {
        Pthread_join(threads[i], NULL);
    }
    printf("parent: end\n");
    return 0;
}

