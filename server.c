#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>

int socket_desc, new_socket, c, valread;
struct sockaddr_in server, client;

struct PCB
{
	int pId;
	int burst;
	int priority;
	int state;
	int timeExecute;
	struct Queue *queue;
};

struct Node
{
	struct Node *next;
	struct PCB process;
};

struct Queue
{
	struct Node *first;

	struct Node *last;
};

struct startJobScheduler
{
	struct Queue *queue;
	int socket;
};

struct startCPUScheduler
{
	struct Queue *r;
	struct Queue *d;
	int algorithm;
	int quantum;
};

void printNode(struct Node *n)
{
	printf("\nProcess ID:%d\nBurst:%d\nPriority:%d\n", n->process.pId, n->process.burst, n->process.priority);
	return;
}

void printQueue(struct Queue *q)
{
	struct Node *tmp = q->first;
	while (tmp != NULL)
	{
		printNode(tmp);
		tmp = tmp->next;
	}
	return;
}

// This process insert a new process into the last spot in the queue
int insertProcess(struct Queue *q, struct PCB pcb)
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
	return 0;
}

// This functions transfers a process node from a ready queue to a done queue
int finishProcess(struct Queue *r, struct Queue *d, int pId)
{
	struct Node *prev = NULL;
	struct Node *tmp = r->first;
	// Checks if the first node is the one we are looking for
	if (tmp->process.pId == pId)
	{
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
	sprintf(string, "%d", number);
	return string;
}

// Split string into array by delimiter
int *splitChar(char string[], char limit[])
{
	char *newString;
	int *array = (int *)malloc(sizeof(int) * 2);

	// Split string by limit
	newString = strtok(string, limit);
	// Convert string into int
	int i = strtol(newString, NULL, 10);
	array[0] = i;
	newString = strtok(NULL, limit);
	i = strtol(newString, NULL, 10);
	array[1] = i;

	return array;
}

// Print time execute by process in console
void printExecution(int timeExecution)
{
	int timer = 0;
	printf("<");
	while (timer < timeExecution)
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
	if (r->first != NULL)
	{
		// Find process by queue order
		struct Node *initNode = r->first;
		int burst = initNode->process.burst;
		int timer = initNode->process.burst - initNode->process.timeExecute;
		printf("Procesando nodo %d\n", initNode->process.pId);
		printNode(initNode);
		// Print process by n time with sleep
		printExecution(timer);
		initNode->process.timeExecute += timer;
		printf("Se ha terminado el proceso #%d con un burst de %d\n", initNode->process.pId, burst);
		finishProcess(r, d, initNode->process.pId);
		// fifo(r, d); NEXT CALL IS MANAGED BY CPU SCHEDULER
	}
	else
	{
		return;
	}
}

// Algorithm by lowest burst
void sjf(struct Queue *r, struct Queue *d)
{
	if (r->first != NULL)
	{
		// Find process by lowest burst
		struct Node *initNode = searchLowestBurstProcess(r);
		int burst = initNode->process.burst;
		int timer = initNode->process.burst - initNode->process.timeExecute;

		// Print process by n time with sleep
		printExecution(timer);
		initNode->process.timeExecute += timer;
		printf("Se ha terminado el proceso #%d con un burst de %d\n", initNode->process.pId, burst);
		finishProcess(r, d, initNode->process.pId);
		// sjf(r, d); NEXT CALL IS MANAGED BY CPU SCHEDULER
	}
	else
	{
		return;
	}
}

// Algorithm by priority
void hpf(struct Queue *r, struct Queue *d)
{
	if (r->first != NULL)
	{
		// Find process by priority
		struct Node *initNode = searchHighestPriorityProcess(r);
		int burst = initNode->process.burst;
		int timer = initNode->process.burst - initNode->process.timeExecute;

		// Print process by n time with sleep
		printExecution(timer);
		initNode->process.timeExecute += timer;
		printf("Se ha terminado el proceso #%d con un burst de %d\n", initNode->process.pId, burst);
		finishProcess(r, d, initNode->process.pId);
		// hpf(r, d); NEXT CALL IS MANAGED BY CPU SCHEDULER
	}
	else
	{
		return;
	}
}

// Algorithm appropiative with quantum
void RoundRobin(struct Queue *r, struct Queue *d, int quantum, int pId)
{
	// Find process to execute
	struct Node *initNode = searchProcessById(r, pId);
	int time = initNode->process.burst - initNode->process.timeExecute;
	// End process or execute by quantum
	if (time > quantum)
	{
		printExecution(quantum);
		printf("Se ha ejecutado el proceso #%d con un quantum de %d\n", initNode->process.pId, quantum);
		// Update time execute
		initNode->process.timeExecute += quantum;
	}
	else
	{
		// Print process by n time with sleep
		printExecution(time);
		initNode->process.timeExecute += time;
		printf("Se ha terminado el proceso #%d con un burst de %d\n", initNode->process.pId, initNode->process.burst);

		// LA TERMINACION MANEJADA POR EL CPU SCHEDULER
		// ESTO PARA QUE NO SE ENCUENTRE CON UN NULO CUANDO LO BUSQUE
		// finishProcess(r, d, initNode->process.pId);
	}
	// RoundRobin(r, d, quantum, pId); NEXT CALL IS MANAGED BY CPU SCHEDULER
}

void *CPU_Scheduler(void *data)
{
	// Inicializacion de los datos para el procesamiento
	struct startCPUScheduler *init_data = (struct startCPUScheduler *)data;
	// Queue de ready
	struct Queue *r = init_data->r;
	// Queue de procesos terminados
	struct Queue *d = init_data->d;
	// Quantum en caso de que se utilice para RR
	int quantum = init_data->quantum;
	// Tipo de algoritmo a utilizar (0->FIFO,1->SJF,2->HPF,3->RR)
	int algorithm = init_data->algorithm;
	// Tiempo en segundos de cpu ocioso
	int cpu_ocioso = 0;
	// Primer nodo a checkear para RR, siempre el primero
	int id = 1;
	// Nodo temporal para el manejo de orden en RR
	struct Node *tmp;
	// Verifica el algoritmo y hace lo procesa
	switch (algorithm)
	{
	case 1:
		while (1)
		{
			if (r->first != NULL)
			{
				// REALIZA EL ALGORITMO FIFO
				fifo(r, d);
			}
			else
			{
				cpu_ocioso += 1;
				sleep(1);
			}
		}
		break;
	case 2:
		while (1)
		{
			if (r->first != NULL)
			{
				// REALIZA EL ALGORITMO SJF
				sjf(r, d);
			}
			else
			{
				cpu_ocioso += 1;
				sleep(1);
			}
		}
		break;
	case 3:
		while (1)
		{
			if (r->first != NULL)
			{
				// REALIZA EL ALGORITMO HPF
				hpf(r, d);
			}
			else
			{
				cpu_ocioso += 1;
				sleep(1);
			}
		}
		break;
	case 4:
		while (1)
		{
			if (r->first != NULL)
			{
				// BUSCA EL NODO ACTUAL
				tmp = searchProcessById(r, id);
				if (tmp == NULL)
				{
					tmp = r->first;
					id = tmp->process.pId;
				}

				// REALIZA EL ALGORITMO
				RoundRobin(r, d, quantum, id);

				if (tmp->next != NULL)
				{
					// SI HAY SIGUIENTE PREPARA SU PROCESAMIENTO

					id = tmp->next->process.pId;
				}
				else
				{

					// SI NO HAY SIGUIENTE PREPARA EL PRIMERO
					id = r->first->process.pId;
				}

				// SI EL PROCESO ANTERIOR TERMINO LO QUITA DEL READY QUEUE
				if (tmp->process.timeExecute >= tmp->process.burst)
				{

					finishProcess(r, d, tmp->process.pId);
				}
			}
			else
			{
				cpu_ocioso += 1;
				sleep(1);
			}
		}
		break;
	default:
		break;
	}
	printf("CPU finalizado");
}

// Insert received process into queue
void *makeProcess(void *pcb)
{
	struct PCB *dataPCB = (struct PCB *)pcb;
	struct PCB processToInsert = {.burst = dataPCB->burst, .pId = dataPCB->pId, .priority = dataPCB->priority, .state = 0, .timeExecute = 0, .queue = NULL};
	insertProcess(dataPCB->queue, processToInsert);
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
	{
		printf("No es posible crear el socket");
		return 1;
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
		return 1;
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
			process->state = 0;
			process->queue = r;

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
}

int getAlgoritm()
{
	int algorit;

	printf("Seleccione el tipo de algoritmo: \n");
	printf("1. FIFO \n");
	printf("2. SJF \n");
	printf("3. HPF \n");
	printf("4. Round Robin \n");
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
	default:
		printf("Invalid choice!\n");
		break;
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
	struct Queue *ready = (struct Queue *)malloc(sizeof(struct Queue));
	struct Queue *done = (struct Queue *)malloc(sizeof(struct Queue));
	// printf("queue done\n\n");
	ready->first = NULL;
	ready->last = NULL;
	done->first = NULL;
	done->last = NULL;
	// struct PCB p1 = {.burst = 4, .pId = 0, .priority = 1, .state = 0, .timeExecute = 0};
	// struct PCB p2 = {.burst = 7, .pId = 1, .priority = 5, .state = 0, .timeExecute = 0};
	// struct PCB p3 = {.burst = 3, .pId = 2, .priority = 3, .state = 0, .timeExecute = 0};
	// struct PCB p4 = {.burst = 5, .pId = 3, .priority = 3, .state = 0, .timeExecute = 0};
	// struct PCB p5 = {.burst = 2, .pId = 4, .priority = 1, .state = 0, .timeExecute = 0};
	// printf("init done\n\n");
	// insertProcess(ready, p1);
	// printf("process insertion 1\n");
	// insertProcess(ready, p2);
	// printf("process insertion 2\n");
	// insertProcess(ready, p3);
	// printf("process insertion 3\n");
	// insertProcess(ready, p4);
	// printf("process insertion 4\n");
	// insertProcess(ready, p5);
	// printf("process insertion 5\n\n");

	// printf("Printing processes in ready queue:");
	// printQueue(ready);

	// printf("\nFinishing process 3\n\n");
	// finishProcess(ready, done, 3);

	// printf("Printing processes in done queue:");
	// printQueue(done);

	// printf("\nPrinting processes in modified ready queue:");
	// printQueue(ready);

	// printf("\nChecking for highest priority process in ready queue:");
	// printNode(searchHighestPriorityProcess(ready));

	// printf("\nChecking for lowest burst process in ready queue:");
	// printNode(searchLowestBurstProcess(ready));
	// printf("\nTests finished\n");

	// Execution of algoritms
	// fifo(ready, done);
	// sjf(ready, done);
	// hpf(ready, done);
	// RoundRobin(ready, done, 2, 0);

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
	pthread_join(job, NULL);
	pthread_join(cpu, NULL);

	return 0;
}
