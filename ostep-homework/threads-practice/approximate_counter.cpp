
#include <time.h>
#include <stdio.h>
#include <unistd.h>

#include "wrapper_function.h"

struct counter_t
{
    int value;
    pthread_mutex_t lock;
};

void init(struct counter_t* counter)
{
    counter->value = 0;
    Pthread_mutex_init(&(counter->lock), NULL);
}

void incremrent(struct counter_t* counter)
{
    Pthread_mutex_lock(&(counter->lock));
    ++(counter->value);
    Pthread_mutex_unlock(&(counter->lock));
    return;
}

void decremrent(struct counter_t* counter)
{
    Pthread_mutex_lock(&(counter->lock));
    --(counter->value);
    Pthread_mutex_unlock(&(counter->lock));
    return;
}

int get(struct counter_t* counter)
{
    Pthread_mutex_lock(&(counter->lock));
    int result = counter->value;
    Pthread_mutex_unlock(&(counter->lock));

    return result;
}

void add(struct counter_t* counter, int value)
{
    Pthread_mutex_lock(&(counter->lock));
    counter->value += value;
    Pthread_mutex_unlock(&(counter->lock));
}

const size_t LOOP_TIME = 1e6;
const long long NS_PER_S = 1e9;
const long long NS_PER_MS = 1e6;
const size_t MAX_THREAD_NUM = 20;
const size_t THRESHOLD = LOOP_TIME / 1e4;

void* work(void* arg)
{
    struct counter_t* counter = static_cast<counter_t*>(arg);
    size_t local_value = 0;
    for (size_t i = 0; i != LOOP_TIME; ++i)
    {
        ++local_value;
        if (local_value >= THRESHOLD)
        {
            add(counter, local_value);
            local_value = 0;
        }
    }

    return NULL;
}

void measure(size_t thread_num)
{
    struct counter_t counter;
    init(&counter);
    printf("----------------------------------------------------------\n");
    printf("thread num: %ld\n", thread_num);
    printf("before loop:\tcounter=%d\n", counter.value);
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (size_t i = 0; i != thread_num; ++i)
    {
        pthread_t thread;
        Pthread_create(&thread, NULL, work, (void*)&counter);
        Pthread_join(thread, NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    long long cost = (end.tv_sec - start.tv_sec) * NS_PER_S + (end.tv_nsec - start.tv_nsec);
    printf("after loop:\tcounter=%d\n", counter.value);
    printf("cost:%lldms\n", cost / NS_PER_MS);
    printf("----------------------------------------------------------\n");

}

int main()
{
    for (size_t i = 1; i <= MAX_THREAD_NUM; ++i)
    {
        measure(i);
    }

}