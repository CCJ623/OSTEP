#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    int x = 100;
    int pid = fork();
    if (pid < 0)
    {
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if (pid == 0)
    {
        // child process
        printf("Child(%d) origin x: %d\n", getpid(), x);
        x = getpid();
        printf("Child(%d) change x to current pid: %d\n", getpid(), x);
    }
    else
    {
        // parent process
        printf("Parent(%d) origin x: %d\n", getpid(), x);
        x = getpid();
        printf("Parent(%d) change x to current pid: %d\n", getpid(), x);
    }
}