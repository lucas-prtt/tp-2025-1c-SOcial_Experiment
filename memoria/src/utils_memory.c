#include "utils_memory.h"
t_list* lista_instrucciones = NULL;

ModulosConectados conexiones;
t_dictionary* instrucciones_por_pid;

int crearSocketConfig(t_config* config, char opcion[]){
    int puertoConfig = config_get_int_value(config, opcion);
    char puerto[7];
    sprintf(puerto, "%d", puertoConfig);
    return crearSocketServer(puerto);
}

void* aceptarConexiones(void* socketPtr) {

    // Guardo el socket de conexion inicialen el stack
    int socket = *(int*)socketPtr; 
    
    // Libero el socket repetido en HEAP
    free(socketPtr);    

    while (1) { // Hasta que se cierre el thread:
        struct sockaddr_in dirCliente;
        socklen_t tamDireccion = sizeof(dirCliente);

        // Acepto una conexion entrante y la guardo en socketCliente
        int socketCliente = accept(socket, (void*)&dirCliente, &tamDireccion);
        if (socketCliente == -1) {
            // Si accept da error
            log_error(logger, "Error al aceptar conexión");
            // Continue hace que termine el ciclo de while sin ejecutar
            // todo lo que sigue y se ejecute otro ciclo a continuacion
            // (Equivalente a un else que envuelva toda la logica posterior
            //  hasta el fin del while)
            continue;
        }

        // Esto no se porque quisieron hacerlo. 
        // Muestra la IP y el Puerto
        char ipCliente[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &dirCliente.sin_addr, ipCliente, INET_ADDRSTRLEN);
        int puertoCliente = ntohs(dirCliente.sin_port);
        log_debug(logger, "Conexión entrante desde %s:%d", ipCliente, puertoCliente);

        // Creo un hilo de atencion y le mando el socketCliente generado en este hilo
        pthread_t hiloCliente;
        int* socketClientePtr = malloc(sizeof(int));
        *socketClientePtr = socketCliente;

        pthread_create(&hiloCliente, NULL, atenderConexion, socketClientePtr);
        pthread_detach(hiloCliente);

    }
    return NULL;
}

void* atenderConexion(void* socketPtr) {
    log_debug(logger, "Hilo de conexion creado para socket %d", *(int*)socketPtr);
    // Cada vez que se hace un conectarSocketClient(ipMemoria, puertoMemoria) 
    // desde otro modulo, se ejecuta esta funcion en un nuevo thread
    
    int id; // El id va a ser el codOp del paquete 
            // (del enum idModulo para el handshake)
    
    t_list * paqueteRecibido = recibir_paquete_lista(*(int*)socketPtr, MSG_WAITALL, &id);
    log_debug(logger, "Paquete recibido: codOp = %d, pointer paquete = %p", id, paqueteRecibido);
    
    // Se recibe un paquete con un codOp SOYKERNEL (0) o SOYCPU (1). (y una lista vacia)

    if (paqueteRecibido == NULL) {
        // Si en vez de lista vacia no tengo la lista 
        // (NULL en lugar de lista de 0 elementos creada con list_create())
        // Entonces: 
        // - No destruyo la lista, pues no existe
        // - Cierro la conexion al socket
        // - Termino con el thread
        log_error(logger, "Se cerró la conexión");
        close(*(int*)socketPtr);
        return NULL;
    }
    // Si el paquete si se recibio...
    log_debug(logger, "El paquete no es nulo");

    // El contenido esta vacio (lista vacia) por lo que
    // la eliminamos para guardar espacio (el ID mantiene su valor)
    list_destroy(paqueteRecibido);

    switch (id) {
        case SOYKERNEL:
            // Si el ID del paquete indica que es el kernel:
            log_info(logger, "## Kernel Conectado - FD del socket: %d", *(int*)socketPtr);
            pthread_t hiloKernel;
            // Creo un thread para atender al kernel y le mando el socket de la conexion
            pthread_create(&hiloKernel, NULL, atenderKernel, socketPtr);
            pthread_detach(hiloKernel);
            break;
        case SOYCPU:
            // Si el ID del paquete indica que es una CPU
            log_info(logger, "Se conectó una CPU.");
            pthread_t hiloCPU;
            // Creo un thread para atender la CPU y le mando el socket de la conexion
            pthread_create(&hiloCPU, NULL, atenderCPU, socketPtr);
            pthread_detach(hiloCPU);
            break;
        default:
            // Si el ID no es ni de CPU ni de Kernel:
            // Hubo algun error
            log_error(logger, "Modulo conectado desconocido. Fallo el Handshake.");
            // No tiene sentido mantener el socket abierto si hubo error, asi que lo cierro
            close(*(int*)socket);
            return NULL;
    }
    close(*(int*)socket);
    // El hilo se cierra tras el handshake que indica el comportamiento necesario
    return NULL; 
}

// busca las instrucciones por PID
t_list* obtener_instrucciones_por_pid(uint32_t pid){
    for (int i = 0; i < list_size(lista_instrucciones); i++)
    {
        t_instrucciones_por_pid* entrada = list_get(lista_instrucciones, i);
        if (entrada->pid == pid){
            return entrada->instrucciones;
        }
    }
    return NULL;
    
}

int obtener_espacio_libre() {
    return 128; // Simulación de 128 páginas libres
}