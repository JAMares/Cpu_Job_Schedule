#include <stdio.h>
#include <time.h>
#include <stdlib.h>
int testReadRand();

int main()
{
    testReadRand();
    return 0;
}

// Placeholder function for file reading and random gen tests
int testReadRand()
{
    int burst, priority, randInt;

    // SEED INIT
    srand(time(NULL));

    FILE *reader = fopen("test.txt", "r");

    // If file does not exist
    if (reader == NULL)
    {
        return 1;
    }

    // checks file for each line
    while (fscanf(reader, "%d %d", &burst, &priority) == 2)
    {
        // Simple random (1-10)
        randInt = rand() % 10;

        // Each time should have a certain amount of sleep
        // Here is where you send the data through the sockets on individual threads
        printf("BURST=%d, PRIORITY=%d, RAND=%d\n", burst, priority, randInt);
    }
    fclose(reader);
    return 0;
}