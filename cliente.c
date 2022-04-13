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

// Message structure
struct Message
{
	int socket;
	char *message;
};

// Stop structure
struct Stop
{
	int stopC;
};

struct Stop stopServer;
int sock = 0;

// Reads keyboard
int getChar()
{
	int pressedK;
	// struct with terminal information
	struct termios staryTermios, novyTermios;
	int actualF, futureF;

	novyTermios = staryTermios;
	// Search for IO signals
	novyTermios.c_lflag &= ~(ICANON);

	// System call with flags
	actualF = fcntl(STDIN_FILENO, F_GETFL);
	futureF = actualF;

	// Non-blocking function
	futureF |= O_NONBLOCK;

	// System call with flags
	fcntl(STDIN_FILENO, F_SETFL, futureF);

	// If a keyboard is pressed
	pressedK = getchar();

	// Client exits
	if (pressedK == 'q')
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
		char *sLength = numberToString(strlen(my_msg->message));
		my_msg->message = ConcatCharToCharArray(my_msg->message, ',');
		strcat(my_msg->message, sLength);
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

		pthread_t thrd;
		char *msgOut = numberToString(burst);
		char *numString = numberToString(priority);
		msgOut = ConcatCharToCharArray(msgOut, ',');
		msgOut = strcat(msgOut, numString);

		struct Message *msg = malloc(sizeof(struct Message));
		msg->message = msgOut;
		msg->socket = socket;

		// Created thread and function call
		pthread_create(&thrd, NULL, sendProcessSocket, (void *)msg);
		pthread_join(thrd, NULL);
		break;
	}
}

// Make random data by socket to server
void randProcess(int socket, int minTime, int maxTime, int minBurst, int maxBurst)
{

	while (stopServer.stopC != 1 & getChar() != 1)
	{
		int burst = rand() % (maxBurst + 1 - minBurst) + minBurst;
		int priority = rand() % (5 + 1 - 1) + 1;
		sendProcess(burst, priority, socket);
		int time = rand() % (maxTime + 1 - minTime) + minTime;
		sleep(time);
		break;
	}
}

// Placeholder function for file reading and random gen tests
int fileRead(int socket, char *txtName)
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

// If CPU creates random process
void *autoCPU(int socket)
{
	int minTime, maxTime, minBurst, maxBurst;
	printf("Input MIN burst range for random when creating process: \n");
	scanf("%d", &minBurst);
	printf("Input MAX burst range for random when creating process: \n");
	scanf("%d", &maxBurst);
	printf("Input MIN sleep time for random when creating process: \n");
	scanf("%d", &minTime);
	printf("Input MAX sleep time for random when creating process: \n");
	scanf("%d", &maxTime);

	while (getChar() != 1 & stopServer.stopC != 1)
	{
		randProcess(socket, minTime, maxTime, minBurst, maxBurst);
	}
}

// If CPU reads process from file
void *manualCPU(int socked)
{
	char archive[100];
	int algorit;
	int q;

	printf("Write file name: \n");
	scanf("%100s", archive);
	fileRead(socked, archive);
}

// main menu
void *mainMenu(int socket)
{
	int choice;

	printf("Main Menu");
	printf("\n\t----------------------");
	printf("\n 1. Automatic CPU");
	printf("\n 2. Run manual CPU");
	printf("\n 3. Exit");
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
	case 3:
		stopServer.stopC = 1;
		printf("Client has closed the program\n");
		close(sock);
		exit(0);
		break;
	default:
		printf("Invalid choice!\n");
		break;
	}
}

int main()
{
	char buffer[2000] = {};
	int valread;
	struct sockaddr_in serv_addr;
	char *msg = "Client sends";

	// SOCKET CREATION
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Socket can not be create\n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	// Convert adress IP
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
	{
		printf("\nInvalid address\n");
		return -1;
	}

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\nConnection Failed\n");
		return -1;
	}

	mainMenu(sock);

	printf("\nClient simulation finished\n");

	close(sock);

	return 0;
}

