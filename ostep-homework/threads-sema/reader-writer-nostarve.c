#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common_threads.h"

//
// Your code goes in the structure and functions below
//
typedef struct __rwlock_t
{
    sem_t lock_;
    sem_t writer_lock_;
    sem_t reader_passport_;
    size_t reader_count_;
} rwlock_t;


void rwlock_init(rwlock_t* rw)
{
    Sem_init(&(rw->lock_), 1);
    Sem_init(&(rw->writer_lock_), 1);
    Sem_init(&(rw->reader_passport_), 1);
    rw->reader_count_ = 0;
}

void rwlock_acquire_readlock(rwlock_t* rw)
{
    Sem_wait(&(rw->reader_passport_));
    Sem_wait(&(rw->lock_));
    if (rw->reader_count_ == 0)
    {
        Sem_wait(&(rw->writer_lock_));
    }
    ++(rw->reader_count_);
    Sem_post(&(rw->lock_));
    Sem_post(&(rw->reader_passport_));
}

void rwlock_release_readlock(rwlock_t* rw)
{
    sleep(1);
    Sem_wait(&(rw->lock_));
    --(rw->reader_count_);
    if (rw->reader_count_ == 0)
    {
        Sem_post(&(rw->writer_lock_));
    }
    Sem_post(&(rw->lock_));
}

void rwlock_acquire_writelock(rwlock_t* rw)
{
    Sem_wait(&(rw->reader_passport_));
    Sem_wait(&(rw->writer_lock_));

}

void rwlock_release_writelock(rwlock_t* rw)
{
    sleep(2);
    Sem_post(&(rw->writer_lock_));
    Sem_post(&(rw->reader_passport_));
}

//
// Don't change the code below (just use it!)
// 

int loops;
int value = 0;

rwlock_t lock;

void* reader(void* arg)
{
    int i;
    for (i = 0; i < loops; i++)
    {
        rwlock_acquire_readlock(&lock);
        printf("read %d\n", value);
        rwlock_release_readlock(&lock);
    }
    return NULL;
}

void* writer(void* arg)
{
    int i;
    for (i = 0; i < loops; i++)
    {
        rwlock_acquire_writelock(&lock);
        value++;
        printf("write %d\n", value);
        rwlock_release_writelock(&lock);
    }
    return NULL;
}

int main(int argc, char* argv[])
{
    assert(argc == 4);
    int num_readers = atoi(argv[1]);
    int num_writers = atoi(argv[2]);
    loops = atoi(argv[3]);

    pthread_t pr[num_readers], pw[num_writers];

    rwlock_init(&lock);

    printf("begin\n");

    int i;
    for (i = 0; i < num_readers; i++)
        Pthread_create(&pr[i], NULL, reader, NULL);
    for (i = 0; i < num_writers; i++)
        Pthread_create(&pw[i], NULL, writer, NULL);

    for (i = 0; i < num_readers; i++)
        Pthread_join(pr[i], NULL);
    for (i = 0; i < num_writers; i++)
        Pthread_join(pw[i], NULL);

    printf("end: value %d\n", value);

    return 0;
}

