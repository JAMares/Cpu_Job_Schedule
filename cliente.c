#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define PROCESS_MAX 20
#define COLS 2
#define PORT 8080

int* randProcess(){
    
    // ARRAY TO SAVE READ DATA
    int* array = (int *)malloc(COLS * sizeof(int));
    
    srand(time(NULL));

    int burst = rand() % (8 + 1 - 3) + 3;
    int priority = rand() % (8 + 1 - 3) + 3;
    array[0] = burst;
    array[1] = priority;

    return array;
}

// Placeholder function for file reading and random gen tests
int** testReadRand()
{

    // ARRAY TO SAVE READ DATA
    int **matrix = (int **)malloc(PROCESS_MAX * sizeof(int*));
    for(int i = 0; i < 5; i++) matrix[i] = (int *)malloc(COLS * sizeof(int));

    int burst, priority, row = 0;

    FILE *reader = fopen("test.txt", "r");

    // If file does not exist
    if (reader == NULL)
    {
        return matrix;
    }

    // checks file for each line
    while (fscanf(reader, "%d %d", &burst, &priority) == 2)
    {
        matrix[row][0] = burst;
        matrix[row][1] = priority;
        row++;

        // Each time should have a certain amount of sleep
        // Here is where you send the data through the sockets on individual threads
        printf("BURST=%d, PRIORITY=%d\n", burst, priority);
    }
    fclose(reader);

    return matrix;
}

// Placeholder function for file reading and random gen tests
int lengthFile()
{
    int line, burst, priority= 0;
    FILE *reader = fopen("test.txt", "r");
    // If file does not exist
    if (reader == NULL)
    {
        return 0;
    }
    // checks file for each line
    while (fscanf(reader, "%d %d", &burst, &priority) == 2)
    {
        line++;
    }
    fclose(reader);
    return line;
}
   
int main(int argc, char const *argv[])
{
    int** data;
    int length_data  = lengthFile();

    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char *msg = "Esto es lo enviado por el cliente";
    char buffer[2000] = {};
    // SOCKET CREATION
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }
   
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
       
    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    send(sock , msg , strlen(msg) , 0 );
    printf("Mensaje enviado\n");
    valread = read( sock , buffer, 2000);
    printf("%s\n",buffer );
	
    return 0;
}