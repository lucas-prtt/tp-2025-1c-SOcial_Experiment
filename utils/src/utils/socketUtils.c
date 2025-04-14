#include "socketUtils.h"

int crearSocket(char* ip, char* puerto, struct addrinfo **server_info){ //Es llamada por crearSocketServer y conectarSocketCliente. No deberia hacer falta usarla
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


void esperarClientes(int socket_server, void (*atenderCliente)(void*), parametrosAtencionThread * params){ 
    // EsperarClientes(): Se queda siempre en esta funcion la cual crea threads. Toma la funcion "atenderCliente"
    // para crear el thread y le aplica los parametros "params". Hay ya hecho un placeholder de la funcion
    // titulado atenderConThread. Hay que completarla para que se pueda usar en el kernel, pero depende fuertemente
    // de como se guarden los datos en el kernel para implementarla

    // AtenderCliente() toma como parametro un puntero a void, que en este caso va a ser a la 
    // estructura infoAtencionThread la cual tiene el socket del cliente y otro puntero que apunta a
    // los "params" en esperarClientes(), que es el struct que se usa para pasar los datos de que cliente se conecto
    // ya sea CPU, IO etc.
    // Al ser un puntero a los "params", estos se pueden modificar dentro de AtenderCliente(), y luego
    // utilizarse donde se halla llamado la funcion esperarClientes, es decir principalmente el main() del modulo kernel
    // PD: Cuando se llame esta funcion en el main del kernel, va a ser necesario llamarla por un thread y terminar el thread una vez se haya conectado todo.
    // Por esto, capaz convendria reordenar los malloc de esta funcion para evitar una mini memory leak cuando se finalice de crear los threads y las conexiones

    //  Se implementa asi:
    //	parametrosAtencionThread * params = malloc(sizeof(params));  // Lugar donde van a agregarse las conexiones
	//  esperarClientes(server_fd, atenderConThread, params);           // Esto, pero ejecutandose en un thread, para poder terminarlo llegado a un cierto punto
    while (1) {
        pthread_t thread;
		infoAtencionThread * infoThread;
		infoThread = malloc(sizeof(infoAtencionThread));
		infoThread->socket = accept(socket_server, NULL, NULL);
		infoThread->parametros = params;
        pthread_create(&thread,
                    NULL,
                    atenderConThread,
                    (void*)infoThread);
        pthread_detach(thread);
    }
}

void * atenderConThread(void * infoThread){   // PLACEHOLDER (Cambiarle el nombre tambien)
	int cliente_fd = /* SOCKET CLIENTE */            (*(infoAtencionThread*)infoThread).socket;
	parametrosAtencionThread * parametros = /*POINTER PARA PASAR PARAMETROS ENTRE THREADS*/     (*(infoAtencionThread*)infoThread).parametros;
                                            // En este caso se lo considera de un struct a definir, pero al ser infoAtencionThread.parametros un (*void), puede cambiar
                                            // Va a tener la informacion para conectar con el kernel
                                            // En este caso, al ser un pointer, los cambios sobre el struct se reflejan en el thread principal
                                            // permitiendo asi la comunicacion.


    // CODIGO QUE RECIBE Y ALMACENA NUEVAS CONEXIONES VA ACA

	free(infoThread); // Libera el pointer infoThread. No asi los parametros 
	pthread_exit(0);  // Termina el thread una vez establecida la conexion con el modulo en cuestion
}
