#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <netinet/in.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define PROCESS_MAX 20
#define COLS 2
#define PORT 8080

struct Message
{
	int socket;
	char *message;
};

struct Stop
{
	int stopC;
};

struct Stop stopServer;
int sock = 0;

int getChar()
{
	int c;
	int oc = '\0';
	struct termios staryTermios, novyTermios;
	int oflags, nflags;

	novyTermios = staryTermios;
	novyTermios.c_lflag &= ~(ICANON);

	oflags = fcntl(STDIN_FILENO, F_GETFL);

	nflags = oflags;
	nflags |= O_NONBLOCK;
	fcntl(STDIN_FILENO, F_SETFL, nflags);
	c = getchar();
	if (c == 'q')
	{
		stopServer.stopC = 1;
		close(sock);
		return 1;
	}
	return 0;
}

// Concat char to string
char *ConcatCharToCharArray(char *Str, char Chr)
{
	int len = strlen(Str);
	char *StrResult = malloc(len + 2);
	while (stopServer.stopC != 1 & getChar() != 1)
	{
		strcpy(StrResult, Str);
		StrResult[len] = Chr;
		StrResult[len + 1] = '\0';
		break;
	}
	return StrResult;
}

// Convert int into String
char *numberToString(int number)
{
	char *string = (char *)malloc(sizeof(char));
	while (stopServer.stopC != 1 & getChar() != 1)
	{
		sprintf(string, "%d", number);
		break;
	}
	return string;
}

// Send to server and wait reply
void *sendProcessSocket(void *msg)
{
	sleep(2);
	while (stopServer.stopC != 1 & getChar() != 1)
	{
		struct Message *my_msg = (struct Message *)msg;
		int valread;
		char buffer[2000] = {};
		// printf("Message: %s\n", my_msg->message);		// Solo para pruebas(ELIMINAR LUEGO)
		// printf("Socket cliente: %d\n", my_msg->socket); // Solo para pruebas(ELIMINAR LUEGO)
		send(my_msg->socket, my_msg->message, strlen(my_msg->message), 0);
		valread = read(my_msg->socket, buffer, 2000);
		puts(buffer);
		break;
	}
}

// Make thread by send data to server
void sendProcess(int burst, int priority, int socket)
{
	while (stopServer.stopC != 1 & getChar() != 1)
	{

		char *msgOut = numberToString(burst);
		char *numString = numberToString(priority);
		msgOut = ConcatCharToCharArray(msgOut, ',');
		msgOut = strcat(msgOut, numString);

		struct Message *msg = malloc(sizeof(struct Message));
		msg->message = msgOut;
		msg->socket = socket;

		// Created thread and function call
		pthread_t thrd;
		pthread_create(&thrd, NULL, sendProcessSocket, (void *)msg);
		pthread_join(thrd, NULL);
		break;
	}
}

// Make random data by socket to server
void randProcess(int socket, int time)
{

	while (stopServer.stopC != 1 & getChar() != 1)
	{
		int burst = rand() % (5 + 1 - 1) + 1;
		int priority = rand() % (5 + 1 - 1) + 1;
		sendProcess(burst, priority, socket);
		sleep(time);
		break;
	}
}

// Placeholder function for file reading and random gen tests
int testReadRand(int socket, char *txtName)
{

	while (stopServer.stopC != 1 & getChar() != 1)
	{
		srand(time(NULL));

		int burst, priority, time;

		FILE *reader = fopen(txtName, "r");

		// If file does not exist
		if (reader == NULL)
		{
			return 0;
		}

		// checks file for each line
		while (fscanf(reader, "%d %d", &burst, &priority) == 2)
		{
			time = rand() % (8 + 1 - 3) + 3;
			// Send data by socket  to server
			sendProcess(burst, priority, socket);
			sleep(time);

			// Each time should have a certain amount of sleep
			// Here is where you send the data through the sockets on individual threads
		}
		fclose(reader);
		break;
	}
	return 1;
}

void *autoCPU(int socket)
{
	int time;

	printf("Ingrese el tiempo de espera entre procesos \n");
	scanf("%d", &time);
	while (stopServer.stopC != 1 & getChar() != 1)
	{
		randProcess(socket, time);
	}
}

void *manualCPU(int socked)
{
	char archive[100];
	int algorit;
	int q;

	printf("Digite el nombre del archivo que desea procesar \n");
	scanf("%100s", archive);
	testReadRand(socked, archive);
}

void *mainMenu(int socket)
{
	int choice;

	printf("Main Menu");
	printf("\n\t----------------------");
	printf("\n 1. Automatic CPU");
	printf("\n 2. Run manual CPU");
	printf("\n Enter your choice \n");
	scanf("%d", &choice);
	switch (choice)
	{
	case 1:
		autoCPU(socket);
		break;
	case 2:
		manualCPU(socket);
		break;
	default:
		printf("Invalid choice!\n");
		break;
	}
}

int main(int argc, char *argv[])
{
	char buffer[2000] = {};
	int valread;
	struct sockaddr_in serv_addr;
	char *msg = "Esto es lo enviado por el cliente";
	// SOCKET CREATION
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Socket creation error \n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	// Convert IPv4 and IPv6 addresses from text to binary form
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
	{
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\nConnection Failed \n");
		return -1;
	}

	mainMenu(sock);

	return 0;
}