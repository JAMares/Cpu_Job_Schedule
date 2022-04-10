#include <stdio.h>
#include <time.h>
#include <unistd.h>

int main()
{
 
    time_t begin = time(NULL);

    sleep(2.5);

    time_t end = time(NULL);

    printf("El tiempo concurrido es de: %d segundos\n", (int)(end-begin));


    return 0;
}