#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>

struct PCB
{
	int pId;
	int burst;
	int priority;
	int state;
	int timeExecute;
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
	while (node->process.pId != pId)
	{
		node = node->next;
	}
	return node;
}

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

void fifo(struct Queue* r, struct Queue* d)
{
	if(r->first != NULL)
	{
		struct Node *initNode = r->first;
		int burst = initNode->process.burst;
		int timer = initNode->process.burst - initNode->process.timeExecute;
		printExecution(timer);
		initNode->process.timeExecute += timer;
		printf("Se ha terminado el proceso #%d con un burst de %d\n", initNode->process.pId, burst);
		finishProcess(r,d,initNode->process.pId);
		fifo(r,d);
	} else {
		return;
	}
}

void sjf(struct Queue* r, struct Queue* d)
{
	if(r->first != NULL)
	{
		struct Node *initNode = searchLowestBurstProcess(r);
		int burst = initNode->process.burst;
		int timer = initNode->process.burst - initNode->process.timeExecute;
		printExecution(timer);
		initNode->process.timeExecute += timer;
		printf("Se ha terminado el proceso #%d con un burst de %d\n", initNode->process.pId, burst);
		finishProcess(r,d,initNode->process.pId);
		sjf(r,d);
	} else {
		return;
	}
}

void hpf(struct Queue* r, struct Queue* d)
{
	if(r->first != NULL)
	{
		struct Node *initNode = searchHighestPriorityProcess(r);
		int burst = initNode->process.burst;
		int timer = initNode->process.burst - initNode->process.timeExecute;
		printExecution(timer);
		initNode->process.timeExecute += timer;
		printf("Se ha terminado el proceso #%d con un burst de %d\n", initNode->process.pId, burst);
		finishProcess(r,d,initNode->process.pId);
		hpf(r,d);
	} else {
		return;
	}
}

void RoundRobin(struct Queue* r, struct Queue* d, int quantum, int pId)
{
	struct Node *initNode =  searchProcessById(r, pId);
	int time = initNode->process.burst - initNode->process.timeExecute;
	if(time > quantum)
	{
		printExecution(quantum);
		printf("Se ha ejecutado el proceso #%d con un quantum de %d\n", initNode->process.pId, quantum);
		initNode->process.timeExecute += quantum;
		if(initNode->next == NULL)
			pId = r->first->process.pId;
		else
			pId = initNode->next->process.pId;
	} else {
		printExecution(time);
		initNode->process.timeExecute += time;
		printf("Se ha terminado el proceso #%d con un burst de %d\n", initNode->process.pId, initNode->process.burst);
		finishProcess(r,d,initNode->process.pId);
		if(r->first == NULL)
			return;
		else if(initNode->next == NULL)
			pId = r->first->process.pId;
		else
			pId = initNode->next->process.pId;
	}
	RoundRobin(r,d, quantum, pId);
}

int main(int argc, char *argv[])
{
	// struct Queue *ready = (struct Queue *)malloc(sizeof(struct Queue));
	// struct Queue *done = (struct Queue *)malloc(sizeof(struct Queue));
	// printf("queue done\n\n");
	// ready->first = NULL;
	// ready->last = NULL;
	// done->first = NULL;
	// done->last = NULL;
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


	
	int socket_desc, new_socket, c, valread;
	char buffer[2000] = {};
	char *msg = "Se envia respuesta del servidor";

	struct sockaddr_in server, client;

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

	// Accept and incoming connection
	puts("Esperando para nuevos clientes...");
	

	


	// Aceptar el socket
	c = sizeof(struct sockaddr_in);
	new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c);
	if (new_socket < 0)
	{
		printf("No se ha aceptado con exito");
		return 1;
	}

	puts("La conexion se ha realizado con exito");

	valread = read(new_socket, buffer, 2000);
	printf("%s\n", buffer);
	send(new_socket, msg, strlen(msg), 0);
	printf("Mensaje enviado\n");
	
	return 0;
}