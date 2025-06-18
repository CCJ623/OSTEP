#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    int fd = open("2.output", O_WRONLY | O_CREAT, 0644);
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
        printf("child writing\n");
        char str[] = "child\n";
        write(fd, str, sizeof(str));
        close(fd);
    }
    else
    {
        // parent process
        printf("Hello from parent(%d)\n", getpid());
        printf("parent writing\n");
        char str[] = "parent\n";
        write(fd, str, sizeof(str));
        close(fd);
    }
}