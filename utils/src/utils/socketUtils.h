#ifndef SOCKETUTILS_HEADER
#define SOCKETUTILS_HEADER
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
typedef struct{ //Usada en esperarClientes()
	int socket;
	void * parametros;
}infoAtencionThread;
typedef struct{ //Ej de uso para pasarle parametros a AtenderCliente
	int ejemplo; // Remplazar "ejemplo" por la informacion necesaria para guardar las conexiones con las CPU, IO, etc.
                    // EJ: lista de sockets de CPUs, id de IO, socket de IO 
}parametrosAtencionThread;


int crearSocket(char* ip, char* puerto, struct addrinfo **servinfo);
int crearSocketServer(char* puerto);
int conectarSocketClient(char* ip, char* puerto);
void esperarClientes(int socket_server, void (*atenderCliente)(void*), parametrosAtencionThread * params);
void * atenderConThread(void * infoThread);







#endif

