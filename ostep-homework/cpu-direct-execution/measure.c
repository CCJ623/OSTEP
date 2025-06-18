#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>
#include <sys/wait.h>

static const size_t ITERATION_TIME = 1e5;
static const unsigned long NSEC_PER_SEC = 1e9;


int main()
{
    printf("measuring system call time:\n");
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (size_t i = 0; i != ITERATION_TIME; ++i)
    {
        read(STDIN_FILENO, NULL, 0);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_t cost = ((end.tv_sec - start.tv_sec) * NSEC_PER_SEC + (end.tv_nsec - start.tv_nsec)) / ITERATION_TIME;
    printf("%dns/call\n", cost);

    printf("measuring context switch time:\n");
    // set particular cpu
    cpu_set_t cpuset;
    int cpu_to_bind = 6;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_to_bind, &cpuset);

    // init pipes
    int pipe1[2], pipe2[2], start_time_pipe[2], end_time_pipe[2];
    pipe(pipe1);
    pipe(pipe2);
    pipe(start_time_pipe);
    pipe(end_time_pipe);

    pid_t process1 = fork();
    if (process1 < 0)
    {
        perror("fork failed");
    }
    else if (process1 == 0)
    {
        // child process
        sched_setaffinity(0, sizeof(cpuset), &cpuset);
        struct timespec time, time_vec[ITERATION_TIME];
        char buffer[1024];
        const char str[] = "OSTEP";
        for (size_t i = 0; i != ITERATION_TIME; ++i)
        {
            write(pipe1[1], str, sizeof(str));
            clock_gettime(CLOCK_MONOTONIC, &time_vec[i]);
            read(pipe2[0], buffer, 1024);
        }

        write(start_time_pipe[1], &time_vec, sizeof(time_vec));
        return 0;
    }

    pid_t process2 = fork();
    if (process2 < 0)
    {
        perror("fork failed");
    }
    else if (process2 == 0)
    {
        // child process
        sched_setaffinity(0, sizeof(cpuset), &cpuset);
        struct timespec time, time_vec[ITERATION_TIME];
        char buffer[1024];
        const char str[] = "OSTEP";
        for (size_t i = 0; i != ITERATION_TIME; ++i)
        {
            read(pipe1[0], buffer, 1024);
            clock_gettime(CLOCK_MONOTONIC, &time_vec[i]);
            write(pipe2[1], str, sizeof(str));
        }

        write(end_time_pipe[1], &time_vec, sizeof(time_vec));
        return 0;
    }

    time_t sum = 0;
    for (size_t i = 0; i != ITERATION_TIME; ++i)
    {
        struct timespec start, end;
        read(start_time_pipe[0], &start, sizeof(struct timespec));
        read(end_time_pipe[0], &end, sizeof(struct timespec));
        sum += (end.tv_sec - start.tv_sec) * NSEC_PER_SEC + (end.tv_nsec - start.tv_nsec);
    }
    printf("%dns/context switch\n", sum / ITERATION_TIME);
}
