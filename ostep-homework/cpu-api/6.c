#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

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
        pid_t return_pid = waitpid(pid, NULL, 0);
        if (return_pid == -1)
        {
            fprintf(stderr, "wait failed\n");
        }
        printf("Hello from parent(%d)\n", getpid());
    }
}