#include <stdio.h>
#include <error.h>
#include <string.h>
#include <stdlib.h>

static const size_t MEGA = 1024 * 1024;

int main(int argc, char* argv[])
{
    if (argc != 3 && argc != 2)
    {
        perror("bad argument");
        exit(-1);
    }

    char* checker_pointer;
    size_t required_size = strtoull(argv[1], &checker_pointer, 10) * MEGA;
    if (*checker_pointer != '\0')
    {
        perror("required size must be interger!");
        exit(-1);
    }

    size_t loop_time = -1;
    if (argc == 3)
    {
        loop_time = strtoull(argv[2], &checker_pointer, 10);
        if (*checker_pointer != '\0')
        {
            perror("loop time must be a interger!");
            exit(-1);
        }
    }

    char* ptr = malloc(required_size);
    memset(ptr, 66, required_size);
    if (ptr == NULL)
    {
        perror("bad malloc!");
        exit(-1);
    }

    if (loop_time == -1)
    {
        while (1)
        {
            for (size_t i = 0; i != required_size; ++i)
            {
                printf("%x", ptr[i]);
            }
        }
    }
    else
    {
        for (size_t count = 0; count != loop_time; ++count)
        {
            for (size_t i = 0; i != required_size; ++i)
            {
                printf("%x", ptr[i]);
            }
        }
    }
}