#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

int socket_desc, new_socket, c, valread, cpu_ocioso;
struct sockaddr_in server, client;
struct Queue *ready;
struct Queue *done;

time_t beginExecution;
time_t endExecution;

// Process PCN
struct PCB
{
	int pId;
	int burst;
	int priority;
	int timeExecute;
	int startTime;
	int endTime;
};

// List nodes
struct Node
{
	struct Node *next;
	struct PCB process;
};

// Queue structure
struct Queue
{
	struct Node *first;
	struct Node *last;
};

// Job Scheduler list structure
struct startJobScheduler
{
	struct Queue *queue;
	int socket;
};

// CPU Scheduler list structure
struct startCPUScheduler
{
	struct Queue *r;
	struct Queue *d;
	int algorithm;
	int quantum;
};

// Indicates functionality has stopped
struct Stop
{
	int stopC;
};

struct Stop stopServer;
int getChar();

// Calculates TAT for each process
int calculateTAT(struct PCB process)
{
	int tat = process.endTime - process.startTime;
	printf("Start time: %d\n", process.startTime);
	printf("End time: %d\n", process.endTime);
	printf("tat proceso %d: %d\n", process.pId, tat);
	return tat;
}

// Calculates WT for each process
int calculateWT(struct PCB process)
{
	int wt = calculateTAT(process) - process.burst;
	printf("wt proceso %d: %d\n", process.pId, wt);
	return wt;
}

// Count each Queue
int countQueue(struct Queue *q)
{
	struct Node *tmp = q->first;
	int count = 0;
	while (tmp != NULL)
	{
		tmp = tmp->next;
		count++;
	}
	return count;
}

// Calculates TAT average
float averageTAT(struct Queue *q)
{
	float sumAll = countQueue(q);
	float average = 0;
	struct Node *tmp = q->first;
	while (tmp != NULL)
	{
		average += calculateTAT(tmp->process);
		tmp = tmp->next;
	}
	return average / sumAll;
}

// Calculates WT average
float averageWT(struct Queue *q)
{
	float sumAll = countQueue(q);
	float average = 0;
	struct Node *tmp = q->first;
	while (tmp != NULL)
	{
		average += calculateWT(tmp->process);
		tmp = tmp->next;
	}
	return average / sumAll;
}

// Prinst nodes of any list
void printNode(struct Node *n)
{
	while (getChar() != 1 & stopServer.stopC != 1)
	{
		printf("\nProcess ID:%d\nBurst:%d\nPriority:%d\n", n->process.pId, n->process.burst, n->process.priority);
		break;
	}
	return;
}

// Prints lists
void printQueue(struct Queue *q)
{
	while (stopServer.stopC != 1 & getChar() != 1)
	{
		struct Node *tmp = q->first;
		while (tmp != NULL)
		{
			printNode(tmp);
			tmp = tmp->next;
		}
		break;
	}
	return;
}

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

	// If key is p prints ready queue
	if (pressedK == 'p')
	{
		printf("\n---------------COLA DE READY------------------\n");
		printf("----------------------------------------------\n");
		printQueue(ready);
		printf("---------------FIN DE COLA DEL READY----------\n");
		printf("----------------------------------------------\n");
	}

	// If key is q exits
	if (pressedK == 'q')
	{
		char *msg = "El servidor ha cesado operaciones\n";
		stopServer.stopC = 1;
		send(new_socket, msg, strlen(msg), 0);
		close(new_socket);
		return 1;
	}

	return 0;
}

// inserta procesos a una lista y crea el PCB
int insertProcess(struct Queue *q, struct PCB pcb)
{
	while (stopServer.stopC != 1 & getChar() != 1)
	{
		if (q->first == NULL)
		{
			q->first = (struct Node *)malloc(sizeof(struct Node));
			q->first->next = NULL;
			q->first->process = pcb;
			q->last = q->first;
		}
		else
		{
			struct Node *tmp = (struct Node *)malloc(sizeof(struct Node));
			tmp->next = NULL;
			tmp->process = pcb;
			q->last->next = tmp;
			q->last = tmp;
		}
		break;
	}
	return 0;
}

// This functions transfers a process node from a ready queue to a done queue
int finishProcess(struct Queue *r, struct Queue *d, int pId)
{
	endExecution = time(NULL);
	while (stopServer.stopC != 1 & getChar() != 1)
	{
		struct Node *prev = NULL;
		struct Node *tmp = r->first;
		// Checks if the first node is the one we are looking for
		if (tmp->process.pId == pId)
		{
			tmp->process.endTime = (int)(endExecution - beginExecution);
			insertProcess(d, tmp->process);
			r->first = tmp->next;
			free(tmp);
			return 0;
		}
		// Sets up search for the rest of the queue
		prev = tmp;
		tmp = tmp->next;
		while (tmp != NULL)
		{
			// If tmp ID matches what we are looking for
			if (tmp->process.pId == pId)
			{
				// Creates a new node in the other list with the same data as tmp
				tmp->process.endTime = (int)(endExecution - beginExecution);
				insertProcess(d, tmp->process);
				// In case tmp is the last node of the queue
				if (tmp->next == NULL)
				{
					// Sets the previous node as the last one
					r->last = prev;
				}
				// Removes tmp from the ready queue and finishes
				prev->next = tmp->next;
				free(tmp);
				return 0;
			}
			// Setup for next iteration
			prev = tmp;
			tmp = tmp->next;
		}
		break;
	}
	return -1;
}

// Function returns a Node containing the highest (lowest number) priority process in the queue
struct Node *searchHighestPriorityProcess(struct Queue *q)
{
	struct Node *tmp = q->first;
	struct Node *highestPriority = q->first;
	while (tmp != NULL)
	{
		if (tmp->process.priority < highestPriority->process.priority)
		{
			highestPriority = tmp;
		}
		tmp = tmp->next;
	}
	return highestPriority;
}

// Function returns a Node containing the shortest (lowest number) burst process in the queue
struct Node *searchLowestBurstProcess(struct Queue *q)
{
	struct Node *tmp = q->first;
	struct Node *lowestBurst = q->first;
	while (tmp != NULL)
	{
		if (tmp->process.burst < lowestBurst->process.burst)
		{
			lowestBurst = tmp;
		}
		tmp = tmp->next;
	}
	return lowestBurst;
}

struct Node *searchProcessById(struct Queue *q, int pId)
{
	struct Node *node = q->first;
	while (node != NULL)
	{
		if (node->process.pId == pId)
		{
			return node;
		}
		node = node->next;
	}
	return NULL;
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

// Split string into array by delimiter
int *splitChar(char string[], char limit[])
{
	char *newString;
	int *array = (int *)malloc(sizeof(int) * 2);

	while (stopServer.stopC != 1 & getChar() != 1)
	{
		// Split string by limit
		newString = strtok(string, limit);
		// Convert string into int
		int i = strtol(newString, NULL, 10);
		array[0] = i;
		newString = strtok(NULL, limit);
		i = strtol(newString, NULL, 10);
		array[1] = i;
		break;
	}

	return array;
}

// Print time execute by process in console
void printExecution(int timeExecution)
{
	int timer = 0;
	printf("<");
	while (timer < timeExecution & stopServer.stopC != 1 & getChar() != 1)
	{
		timer++;
		printf("-------,");
		sleep(1);
	}
	printf(">\n");
}

// Algorithm by queue order
void fifo(struct Queue *r, struct Queue *d)
{
	while (stopServer.stopC != 1 & getChar() != 1)
	{
		if (r->first != NULL)
		{
			// Find process by queue order
			struct Node *initNode = r->first;
			int burst = initNode->process.burst;
			int timer = initNode->process.burst - initNode->process.timeExecute;
			printf("Procesando Nodo %d:", initNode->process.pId);
			printNode(initNode);
			// Print process by n time with sleep
			printExecution(timer);
			initNode->process.timeExecute += timer;
			printf("Se ha terminado el proceso #%d con un burst de %d\n", initNode->process.pId, burst);
			finishProcess(r, d, initNode->process.pId);
			return;
		}
		else
		{
			return;
		}
	}
}

// Algorithm by lowest burst
void sjf(struct Queue *r, struct Queue *d)
{
	while (stopServer.stopC != 1 & getChar() != 1)
	{
		if (r->first != NULL)
		{
			// Find process by lowest burst
			struct Node *initNode = searchLowestBurstProcess(r);
			int burst = initNode->process.burst;
			int timer = initNode->process.burst - initNode->process.timeExecute;
			printf("Procesando Nodo %d:", initNode->process.pId);
			printNode(initNode);
			// Print process by n time with sleep
			printExecution(timer);
			initNode->process.timeExecute += timer;
			printf("Se ha terminado el proceso #%d con un burst de %d\n", initNode->process.pId, burst);
			finishProcess(r, d, initNode->process.pId);
			return;
		}
		else
		{
			return;
		}
	}
}

// Algorithm by priority
void hpf(struct Queue *r, struct Queue *d)
{
	while (stopServer.stopC != 1 & getChar() != 1)
	{
		if (r->first != NULL)
		{
			// Find process by priority
			struct Node *initNode = searchHighestPriorityProcess(r);
			int burst = initNode->process.burst;
			int timer = initNode->process.burst - initNode->process.timeExecute;
			printf("Procesando Nodo %d:", initNode->process.pId);
			printNode(initNode);
			// Print process by n time with sleep
			printExecution(timer);
			initNode->process.timeExecute += timer;
			printf("Se ha terminado el proceso #%d con un burst de %d\n", initNode->process.pId, burst);
			finishProcess(r, d, initNode->process.pId);
			return;
		}
		else
		{
			return;
		}
	}
}

// Algorithm appropiative with quantum
void RoundRobin(struct Queue *r, struct Queue *d, int quantum, int pId)
{
	while (stopServer.stopC != 1 & getChar() != 1)
	{
		// Find process to execute
		struct Node *initNode = searchProcessById(r, pId);
		int time = initNode->process.burst - initNode->process.timeExecute;
		printf("Procesando Nodo %d:", initNode->process.pId);
		printNode(initNode);
		// End process or execute by quantum
		if (time > quantum & stopServer.stopC != 1 & getChar() != 1)
		{
			printExecution(quantum);
			printf("Se ha ejecutado el proceso #%d con un quantum de %d\n", initNode->process.pId, quantum);
			// Update time execute
			initNode->process.timeExecute += quantum;
			return;
		}
		else if (getChar() != 1 & stopServer.stopC != 1)
		{
			// Print process by n time with sleep
			printExecution(time);
			initNode->process.timeExecute += time;
			printf("Se ha terminado el proceso #%d con un burst de %d\n", initNode->process.pId, initNode->process.burst);
			return;
		}
	}
}

void *CPU_Scheduler(void *data)
{
	// Initializes data for procedures
	struct startCPUScheduler *init_data = (struct startCPUScheduler *)data;
	// Initialization of Ready queue
	struct Queue *r = init_data->r;
	// Initialization of Done queue
	struct Queue *d = init_data->d;
	// Quantum used for Round Robin
	int quantum = init_data->quantum;
	// Type of algorithm to use (0->FIFO,1->SJF,2->HPF,3->RR)
	int algorithm = init_data->algorithm;
	// CPU Idle time counter
	cpu_ocioso = 0;
	// Sets up first node for Round Robin
	int id = 1;
	// Temp node setup for Round Robin
	struct Node *tmp;

	// Verify algorithm choice and run cicle
	switch (algorithm)
	{
	case 1:
		while (getChar() != 1 & stopServer.stopC != 1)
		{
			// Checks that there is a node in the Ready queue
			if (r->first != NULL)
			{
				// Executes FIFO algorithm
				fifo(r, d);
			}
			else
			{
				// Increases CPU Idle time counter and sleeps for one second
				cpu_ocioso += 1;
				sleep(1);
			}
		}
		break;
	case 2:
		while (getChar() != 1 & stopServer.stopC != 1)
		{
			// Checks that there is a node in the Ready queue
			if (r->first != NULL)
			{
				// Executes SJF algorithm
				sjf(r, d);
			}
			else
			{
				// Increases CPU Idle time counter and sleeps for one second
				cpu_ocioso += 1;
				sleep(1);
			}
		}
		break;
	case 3:
		while (getChar() != 1 & stopServer.stopC != 1)
		{
			// Checks that there is a node in the Ready queue
			if (r->first != NULL)
			{
				// Executes HPF algorithm
				hpf(r, d);
			}
			else
			{
				// Increases CPU Idle time counter and sleeps for one second
				cpu_ocioso += 1;
				sleep(1);
			}
		}
		break;
	case 4:
		while (getChar() != 1 & stopServer.stopC != 1)
		{
			// Checks that there is a node in the Ready queue
			if (r->first != NULL)
			{
				// Searches for node to execute with stored id value
				tmp = searchProcessById(r, id);

				// If the node no longers exists, set the node to first in Ready queue
				if (tmp == NULL)
				{
					tmp = r->first;
					id = tmp->process.pId;
				}

				// Execute Round Robin algorithm
				RoundRobin(r, d, quantum, id);

				// Check if there is a next node
				if (tmp->next != NULL)
				{
					// If there is a next, change the stored id to its pId

					id = tmp->next->process.pId;
				}
				else
				{

					// If there isn't a next, change the stored id to the first's node in the queue
					id = r->first->process.pId;
				}

				// If the executed process has already finished, transfer it to the done queue
				if (tmp->process.timeExecute >= tmp->process.burst)
				{
					finishProcess(r, d, tmp->process.pId);
				}
			}
			else
			{
				// Increases CPU Idle time counter and sleeps for one second
				cpu_ocioso += 1;
				sleep(1);
			}
		}
		break;
	default:
		break;
	}
}

// Insert received process into queue
void *makeProcess(void *pcb)
{
	struct PCB *dataPCB = (struct PCB *)pcb;
	if (dataPCB->pId == 1)
		beginExecution = time(NULL);
	endExecution = time(NULL);
	int initTime = (int)(endExecution - beginExecution);
	struct PCB processToInsert = {.burst = dataPCB->burst, .pId = dataPCB->pId, .priority = dataPCB->priority, .timeExecute = 0, .startTime = initTime};
	insertProcess(ready, processToInsert);
}

// Insert received process into CPU queue
void *JOB_Scheduler(void *launch_data)
{
	// Vars to read from client socket and insert process
	struct startJobScheduler *my_job_launch = (struct startJobScheduler *)launch_data;
	// int socket = my_job_launch->socket;
	struct Queue *r = my_job_launch->queue;
	char buffer1[2000] = {};
	char msg[32] = "Se crea el proceso con el PID #";
	char limit[] = ",";
	char *answer;
	int *dataPCB;
	int pID = 1;
	int valread = 0;
	char *sPID;
	pthread_t thrd;

	int read_size;

	char buffer[2000] = {};

	// Create socket
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1)
		// Cycle to continue reading from client socket
		while ((read_size = recv(new_socket, buffer, 2000, 0)) > 0 & getChar() != 1 & stopServer.stopC != 1)
		{
			printf("No es posible crear el socket");
			return NULL;
		}

	// Prepare the sockaddr_in structure
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8080);

	// Bind
	if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		printf("bind falla");
		return NULL;
	}
	puts("El bind se conecta con exito");

	// Listen
	listen(socket_desc, 3);

	// Aceptar el socket
	c = sizeof(struct sockaddr_in);
	while (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c))
	{
		puts("\nLa conexion se ha realizado con exito\n");

		// Cycle to continue reading from client socket
		while ((read_size = recv(new_socket, buffer1, 2000, 0)) > 0)
		{
			sPID = numberToString(pID);
			dataPCB = splitChar(buffer1, limit);

			struct PCB *process = malloc(sizeof(struct PCB));

			process->burst = dataPCB[0];
			process->priority = dataPCB[1];
			process->pId = pID;
			process->timeExecute = 0;

			// Thread insert process into ready queue
			pthread_create(&thrd, NULL, makeProcess, (void *)process);
			pthread_join(thrd, NULL);

			// Server socket replies process id created
			strcpy(msg, "Se crea el proceso con el PID #");
			answer = strcat(msg, sPID);
			send(new_socket, answer, strlen(answer), 0);
			pID++;
		}
		printf("\nConexion del cliente finalizada, esperando nueva conexion...\n");
	}
	printf("\nJob Scheduler Finalizo\n");
	// printf("Muere Job Scheduler\n");
}

int getAlgoritm()
{
	int algorit;

	printf("Seleccione el tipo de algoritmo: \n");
	printf("1. FIFO \n");
	printf("2. SJF \n");
	printf("3. HPF \n");
	printf("4. Round Robin \n");
	printf("5. Exit \n");
	scanf("%d", &algorit);
	switch (algorit)
	{
	case 1:
		return 1;
		break;
	case 2:
		return 2;
		break;
	case 3:
		return 3;
		break;
	case 4:
		return 4;
		// printf("Inserte el q");
		// scanf("%d",&q);
		break;
	case 5:
		char *msg = "El servidor ha cesado operaciones\n";
		stopServer.stopC = 1;
		send(new_socket, msg, strlen(msg), 0);
		close(new_socket);
		exit(0);
	default:
		printf("Invalid choice!\n");
		getAlgoritm();
	}
}

int quamt()
{
	int qm;

	printf("Ingrese el tiempo de RR: \n");
	scanf("%d", &qm);
	return qm;
}

int main(int argc, char *argv[])
{
	stopServer.stopC = 0;

	ready = (struct Queue *)malloc(sizeof(struct Queue));
	done = (struct Queue *)malloc(sizeof(struct Queue));
	// printf("queue done\n\n");
	ready->first = NULL;
	ready->last = NULL;
	done->first = NULL;
	done->last = NULL;

	struct startJobScheduler *launch = malloc(sizeof(struct startJobScheduler));
	launch->queue = ready;
	launch->socket = new_socket;
	struct startCPUScheduler *cpuData = malloc(sizeof(struct startCPUScheduler));
	cpuData->r = ready;
	cpuData->d = done;
	cpuData->algorithm = getAlgoritm();

	if (cpuData->algorithm == 4)
	{
		cpuData->quantum = quamt();
	}

	pthread_t job, cpu;

	pthread_create(&job, NULL, JOB_Scheduler, (void *)launch);
	pthread_create(&cpu, NULL, CPU_Scheduler, (void *)cpuData);

	while (getChar() != 1 & stopServer.stopC != 1)
	{
		continue;
	}

	float tat = averageTAT(done);
	float wt = averageWT(done);

	printf("TAT Average de Procesos: %f\n", tat);
	printf("WT Average de Procesos: %f\n", wt);

	return 0;
}
