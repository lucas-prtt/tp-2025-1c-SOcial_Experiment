#include "socketUtils.h"

int crearSocket(char* ip, char* puerto, struct addrinfo * * server_info){ //Es llamada por crearSocketServer y conectarSocketCliente. No deberia hacer falta usarla
    // Recordar que es necesario server_info como pointer a pointer ya que el pointer es modificado al alocar memoria en getaddrinfo()
    int soc;
    struct addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // IGNORADA SI ip != NULL (en cliente)

	getaddrinfo(ip, puerto, &hints, server_info);
    soc = socket((*server_info)->ai_family, (*server_info)->ai_socktype, (*server_info)->ai_protocol);
    setsockopt(soc, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));
    return soc;
}

int crearSocketServer(char* puerto){ //Devuelve un socket listo para conectar con los clientes
    int soc;
    struct addrinfo * serverInfo = NULL;
    soc = crearSocket(NULL, puerto, &serverInfo);
    bind(soc, serverInfo->ai_addr, serverInfo->ai_addrlen);
	listen(soc, SOMAXCONN);
	freeaddrinfo(serverInfo);
    return soc;
}

int conectarSocketClient(char* ip, char* puerto){//Devuelve un socket listo para comunicarse con el server
    int soc;
    struct addrinfo * serverInfo = NULL;
    soc = crearSocket(ip, puerto, &serverInfo);
    connect(soc, serverInfo->ai_addr, serverInfo->ai_addrlen);
	freeaddrinfo(serverInfo);
    return soc;
}

int esperarClientes(int socket_server, void (*atenderCliente)(void*)){ //EsperarClientes(): Se queda siempre en esta funcion la cual crea threads. Toma la funcion atenderCliente para crear el thread "
    while (1) { //Copiada de utnso.com (guia sockets), hay que ver si anda
        pthread_t thread;
        int *fd_conexion_ptr = malloc(sizeof(int));
        *fd_conexion_ptr = accept(socket_server, NULL, NULL);
        pthread_create(&thread,
                    NULL,
                    (void*) atenderCliente,
                    fd_conexion_ptr);
        pthread_detach(thread);
    }
}

