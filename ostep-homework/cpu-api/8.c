#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    int pipefd[2];
    pipe(pipefd);
    int writer = fork();
    if (writer < 0){
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if (writer == 0)
    {
        // writer process
        printf("Hello from writer(%d)\n", getpid());
        printf("writing to pipe\n");
        char str[] = "[Hi! I am writer!]\n";
        write(pipefd[1], str, sizeof(str));
        return 0;
    }

    int reader = fork();
    if (reader < 0)
    {
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if (reader == 0)
    {
        // reader process
        waitpid(writer, NULL, 0);
        printf("Hello from reader(%d)\n", getpid());
        printf("reading from pipe\n");
        char buffer[1024];
        read(pipefd[0], &buffer, 1024);
        printf("%s", buffer);
        return 0;
    }
}