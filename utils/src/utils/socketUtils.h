#ifndef SOCKETUTILS_HEADER
#define SOCKETUTILS_HEADER
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>


int crearSocket(char* ip, char* puerto, struct addrinfo **servinfo);
int crearSocketServer(char* puerto);
int conectarSocketClient(char* ip, char* puerto);
void liberarConexion(int socket);







#endif

