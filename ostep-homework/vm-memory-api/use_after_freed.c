#include <stdlib.h>
#include <stdio.h>

int main()
{
    int* ptr = malloc(100);
    free(ptr);
    printf("%d\n", ptr[6]);
    *ptr = 666;

    return 0;
}