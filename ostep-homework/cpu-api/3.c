#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    int pid = fork();
    if (pid < 0)
    {
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if (pid == 0)
    {
        // child process
        printf("Hello from child(%d)\n", getpid());
    }
    else
    {
        // parent process
        sleep(1);
        printf("Hello from parent(%d)\n", getpid());
    }
}