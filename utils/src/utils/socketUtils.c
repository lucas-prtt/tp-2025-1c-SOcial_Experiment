#include "socketUtils.h"

int crearSocket(char* ip, char* puerto, struct addrinfo **server_info) { //Es llamada por crearSocketServer y conectarSocketCliente. No deberia hacer falta usarla
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
    if (soc == -1)
        freeaddrinfo(serverInfo);
        return soc;
    bind(soc, serverInfo->ai_addr, serverInfo->ai_addrlen);
	listen(soc, SOMAXCONN);
	freeaddrinfo(serverInfo);
    return soc;
}

int conectarSocketClient(char* ip, char* puerto){ //Devuelve un socket listo para comunicarse con el server
    int soc;
    struct addrinfo * serverInfo = NULL;
    soc = crearSocket(ip, puerto, &serverInfo);
    if (soc == -1)
        freeaddrinfo(serverInfo);
        return soc;
    int err = connect(soc, serverInfo->ai_addr, serverInfo->ai_addrlen);
	freeaddrinfo(serverInfo);
    if (err == -1)
        return err;
    return soc;
}

int estaConexionDisponible(char* ip, char* puerto) {
        int nuevoSocket;
        nuevoSocket = conectarSocketClient(ip, puerto);
        if(nuevoSocket == -1) {
            return 0;
        }
        else {
            liberarConexion(nuevoSocket);
            return 1;
        }
}

void liberarConexion(int socket) {
    close(socket);
}

void liberarConexionPuntero(void * socket) {
    close(*(int*)socket);
}