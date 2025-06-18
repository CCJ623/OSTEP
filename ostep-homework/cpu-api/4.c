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
        execl("/bin/ls", "ls", "-a", NULL);
        printf("Won't print this line!\n");
    }
    else
    {
        // parent process
        printf("Hello from parent(%d)\n", getpid());
    }

    pid = fork();
    if (pid < 0)
    {
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if (pid == 0)
    {
        // child process
        printf("Hello from child(%d)\n", getpid());
        char* vec[] = { "ls", "-a", NULL};
        execv("/bin/ls", vec);
        printf("Won't print this line!\n");
    }
    else
    {
        // parent process
        printf("Hello from parent(%d)\n", getpid());
    }

    pid = fork();
    if (pid < 0)
    {
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if (pid == 0)
    {
        // child process
        printf("Hello from child(%d)\n", getpid());
        execlp("ls", "ls", "-a", NULL);
        printf("Won't print this line!\n");
    }
    else
    {
        // parent process
        printf("Hello from parent(%d)\n", getpid());
    }

    pid = fork();
    if (pid < 0)
    {
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if (pid == 0)
    {
        // child process
        printf("Hello from child(%d)\n", getpid());
        char* vec[] = { "ls", "-a", NULL};
        execvp("ls", vec);
        printf("Won't print this line!\n");
    }
    else
    {
        // parent process
        printf("Hello from parent(%d)\n", getpid());
    }

    pid = fork();
    if (pid < 0)
    {
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if (pid == 0)
    {
        // child process
        printf("Hello from child(%d)\n", getpid());
        char *envp[] = {"MY_VAR=hello", "ANOTHER_VAR=world", NULL};
        execle("/usr/bin/env", "env", (char *)NULL, envp);
        printf("Won't print this line!\n");
    }
    else
    {
        // parent process
        printf("Hello from parent(%d)\n", getpid());
    }

    pid = fork();
    if (pid < 0)
    {
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if (pid == 0)
    {
        // child process
        printf("Hello from child(%d)\n", getpid());
        char *envp[] = {"MY_VAR=hello", "ANOTHER_VAR=world", NULL};
        execle("/usr/bin/env", "env", NULL, envp);
        printf("Won't print this line!\n");
    }
    else
    {
        // parent process
        printf("Hello from parent(%d)\n", getpid());
    }
}