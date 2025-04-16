#ifndef SOCKETUTILS_HEADER
#define SOCKETUTILS_HEADER
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>

int crearSocket(char* ip, char* puerto, struct addrinfo **server_info);
int crearSocketServer(char* puerto);
int conectarSocketClient(char* ip, char* puerto);
int estaConexionDisponible(char* ip, char* puerto); // Crea una conexion mediante un socket. Si sale bien libera el socket y devuelve 1. De lo contrario devuelve 0. Puede tardar mucho en funcion del timeout del connect(). Evitar su uso fuera de threads para no bloquear el hilo principal, excepto que eso sea lo que se desea.
void liberarConexion(int socket);
void liberarConexionPuntero(void * socket);






#endif

