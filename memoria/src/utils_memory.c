#include "utils_memory.h"

ModulosConectados conexiones;

int crearSocketConfig(t_config* config, char opcion[]){
    int puertoConfig = config_get_int_value(config, opcion);
    char puerto[7];
    sprintf(puerto, "%d", puertoConfig);
    return crearSocketServer(puerto);
}

void* aceptarConexiones(void* socketPtr) {

    int socket = *(int*)socketPtr;
    free(socketPtr);

    while (1) {
        struct sockaddr_in dirCliente;
        socklen_t tamDireccion = sizeof(dirCliente);

        int socketCliente = accept(socket, (void*)&dirCliente, &tamDireccion);
        if (socketCliente == -1) {
            log_error(logger, "Error al aceptar conexión");
            continue;
        }

        char ipCliente[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &dirCliente.sin_addr, ipCliente, INET_ADDRSTRLEN);
        int puertoCliente = ntohs(dirCliente.sin_port);

        log_info(logger, "Conexión entrante desde %s:%d", ipCliente, puertoCliente);

        // Por el momento no voy a atender la conexion hasta que utilizemos handshake, acepto las conexiones pero todavia no detecto que modulo se esta conectando

        // pthread_t hiloCliente;
        // int* socketClientePtr = malloc(sizeof(int));
        // *socketClientePtr = socketCliente;

        // pthread_create(&hiloCliente, NULL, atenderConexion, socketClientePtr);
        // pthread_detach(hiloCliente);

    }
    return NULL;
}

void* atenderConexion(void* socketPtr) {
    int socket = *(int*)socketPtr;
    ID_MODULO id;
    t_list * paqueteRecibido = recibir_paquete_lista(socket, MSG_WAITALL, &id);
    if (paqueteRecibido == NULL) {
        log_error(logger, "Se cerró la conexión");
        close(socket);
        return NULL;}
    switch (id) {
        case SOYKERNEL:
            log_info(logger, "Se conectó el Kernel.");
            pthread_t hiloKernel;
            pthread_create(&hiloKernel, NULL, atenderKernel, socketPtr);
            pthread_detach(hiloKernel);
            break;
        case SOYCPU:
            log_info(logger, "Se conectó una CPU.");
            break;
        default:
            log_error(logger, "Modulo desconocido.");
            close(socket);
            return NULL;
    }
    close(socket);
    return NULL;
}