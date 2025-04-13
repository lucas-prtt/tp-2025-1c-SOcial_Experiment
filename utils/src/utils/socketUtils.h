#ifndef SOCKETUTILS_HEADER
#define SOCKETUTILS_HEADER
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

int crearSocket(char* ip, char* puerto, struct addrinfo *servinfo);
int crearSocketServer(char* puerto);
int conectarSocketClient(char* ip, char* puerto);

int esperarClientes(int socket_server, void (*atenderCliente)(void*));








#endif

