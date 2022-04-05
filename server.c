#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
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
    struct Node* next;
    struct PCB process;
};

struct Queue
{
	struct Node* first;

	struct Node* last;
};



int main(int argc , char *argv[])
{
	int socket_desc , new_socket , c, valread;
	char buffer[2000] = {};
    char *msg = "Se envia respuesta del servidor";
	
	struct sockaddr_in server , client;
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("No es posible crear el socket");
		return 1;
	}
	
	//Prepare the sockaddr_in structure
	memset( &server, 0, sizeof(server) );
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 8080 );
	
	//Bind
	if(bind(socket_desc,(struct sockaddr *) &server , sizeof(server)) < 0)
	{
		printf("bind falla");
		return 1;
	}
	puts("El bind se conecta con exito");
	
	//Listen
	listen(socket_desc , 3);
	
	//Accept and incoming connection
	puts("Esperando para nuevos clientes...");
	
	//Aceptar el socket
	c = sizeof(struct sockaddr_in);
	new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	if (new_socket<0)
	{
		printf("No se ha aceptado con exito");
		return 1;
	}
	
	puts("La conexion se ha realizado con exito");

	valread = read( new_socket , buffer, 2000);
    printf("%s\n",buffer );
    send(new_socket , msg , strlen(msg) , 0 );
    printf("Mensaje enviado\n");
	
	return 0;
}