#define _GNU_SOURCE

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>

static const unsigned long long NS_PER_S = 1e9;

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        perror("the number of arguments must be 3!");
        exit(-1);
    }

    char* check_ptr = NULL;
    size_t num_pages = strtoull(argv[1], &check_ptr, 10);
    if (*check_ptr != '\0')
    {
        perror("the number of pages must be an unsigned interger!");
        exit(-1);
    }

    size_t num_trials = strtoull(argv[2], &check_ptr, 10);
    if (*check_ptr != '\0')
    {
        perror("the number of trials must be an unsigned interger!");
        exit(-1);
    }

    // set to specific CPU
    int core_id = 6;
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(core_id, &cpu_set);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set) == -1)
    {
        perror("failed to set to specific CPU\n");
        return 1;
    }

    size_t page_size = sysconf(_SC_PAGESIZE);
    int* array = calloc(page_size * num_pages / sizeof(int), sizeof(int));

    struct timespec start_time, end_time;
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &start_time) == -1)
    {
        perror("failed to get start time!");
        exit(1);
    }

    for (size_t trials_cnt = 0; trials_cnt != num_trials; ++trials_cnt)
    {
        for (size_t page_cnt = 0; page_cnt != num_pages; ++page_cnt)
        {
            int* access_address = (void*)array + (page_cnt * page_size);
            (*access_address) += 1;
        }
    }

    if (clock_gettime(CLOCK_MONOTONIC_RAW, &end_time) == -1)
    {
        perror("failed to get end time!");
        exit(1);
    }

    double cost = (double)((end_time.tv_sec - start_time.tv_sec) * NS_PER_S +
        (end_time.tv_nsec - start_time.tv_nsec));
    printf("%f ns in total\n", cost);
    printf("%lu trials\n", num_trials);
    printf("%lu pages/trial\n", num_pages);
    printf("%f ns/trial\n", cost / num_trials);
    printf("%f ns/page\n", cost / (num_pages * num_trials));

    return 0;
}
