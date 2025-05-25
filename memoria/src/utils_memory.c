#include "utils_memory.h"
#include <arpa/inet.h>

t_list* procesos;
ModulosConectados conexiones;

int crearSocketConfig(t_config* config, char opcion[]){
    int puertoConfig = config_get_int_value(config, opcion);
    char puerto[7];
    sprintf(puerto, "%d", puertoConfig);
    return crearSocketServer(puerto);
}

void* aceptarConexiones(void* socketPtr) {

    log_info(logger, "Hilo aceptarConexiones iniciado.");

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

        log_info(logger, "Esperando conexión entrante...");

        char ipCliente[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &dirCliente.sin_addr, ipCliente, INET_ADDRSTRLEN);
        int puertoCliente = ntohs(dirCliente.sin_port);

        log_info(logger, "Conexión entrante desde %s:%d", ipCliente, puertoCliente);

        // Por el momento no voy a atender la conexion hasta que utilizemos handshake, acepto las conexiones pero todavia no detecto que modulo se esta conectando

        pthread_t hiloCliente;
        int* socketClientePtr = malloc(sizeof(int));
        *socketClientePtr = socketCliente;

        pthread_create(&hiloCliente, NULL, atenderConexion, socketClientePtr);
        pthread_detach(hiloCliente);

    }
    return NULL;
}

void* atenderConexion(void* socketPtr) {
    int socket = *(int*)socketPtr;
    free(socketPtr);
    ID_MODULO id;
    int bytesRecibidos = recv(socket, &id, sizeof(ID_MODULO), 0);
    if (bytesRecibidos <= 0) {
        log_error(logger, "No se recibió ningún handshake (o se cerró la conexión)");
        close(socket);
        return NULL;
    }
    switch (id) {
        case SOYKERNEL:
            log_info(logger, "Se conectó el Kernel.");

            // Recibir el PID
            uint32_t pid, pc;
            if (recv(socket, &pid, sizeof(uint32_t), 0) <= 0) {
                log_error(logger, "Error al recibir PID desde Kernel.");
                close(socket);
                return NULL;
            }

            // Construir el path del archivo de pseudocódigo
            char path[100];
            sprintf(path, "./pseudocodigo/%d.txt", pid); // Asegurate de tener esta carpeta y los archivos .txt

            FILE* archivo = fopen(path, "r");
            if (!archivo) {
                log_error(logger, "No se pudo abrir archivo de pseudocódigo para PID %d", pid);
                close(socket);
                return NULL;
            }

            // Leer instrucciones del archivo
            t_list* instrucciones = list_create();
            char* linea = NULL;
            size_t len = 0;
            while (getline(&linea, &len, archivo) != -1) {
                linea[strcspn(linea, "\n")] = 0;  // Eliminar el '\n'
                list_add(instrucciones, strdup(linea));
            }
            free(linea);
            fclose(archivo);

            // Crear estructura del proceso y guardarlo
            Proceso* nuevo = malloc(sizeof(Proceso));
            nuevo->pid = pid;
            nuevo->instrucciones = instrucciones;
            list_add(procesos, nuevo);

            log_info(logger, "## PID: %d - Proceso Creado - Tamaño: %d", pid, list_size(instrucciones));

            // Enviar OK al Kernel
            uint32_t ok = 1;
            send(socket, &ok, sizeof(uint32_t), 0);

            break;
        case SOYCPU:
            log_info(logger, "Se conectó una CPU.");

            // Recibir PID y PC desde CPU
            recv(socket, &pid, sizeof(uint32_t), 0);
            recv(socket, &pc, sizeof(uint32_t), 0);

            // Buscar el proceso
            Proceso* encontrado = NULL;
            for (int i = 0; i < list_size(procesos); i++) {
                Proceso* p = list_get(procesos, i);
                if (p->pid == pid) {
                    encontrado = p;
                    break;
                }
            }

            if (!encontrado) {
                log_error(logger, "PID %d no encontrado al pedir instrucción", pid);
                close(socket);
                return NULL;
            }

            // Buscar la instrucción
            if (pc >= list_size(encontrado->instrucciones)) {
                log_error(logger, "PC %d inválido para PID %d", pc, pid);
                close(socket);
                return NULL;
            }

            char* instruccion = list_get(encontrado->instrucciones, pc);

            // Enviar la instrucción
            uint32_t tamanio = strlen(instruccion) + 1;
            send(socket, &tamanio, sizeof(uint32_t), 0);
            send(socket, instruccion, tamanio, 0);

            log_info(logger, "## PID: %d - Obtener instrucción: %d - Instrucción: %s", pid, pc, instruccion);

            break;
        case PEDIR_ESPACIO_LIBRE:
            log_info(logger, "Solicitud de espacio libre recibida");
            uint32_t espacio_libre = 1024;
            send(socket, &espacio_libre, sizeof(uint32_t), 0);
            break;
        default:
            log_error(logger, "Modulo desconocido.");
            close(socket);
            return NULL;
    }
    close(socket);
    return NULL;
}

void cargarInstrucciones(uint32_t pid){ // funcion para leer archivo de pseudocódigo por PID
    char ruta[100];
    sprintf(ruta, "./pseudocodigo/%d.txt", pid); // ajusta ruta según la estructura

    FILE* archivo = fopen(ruta, "r");
    if (!archivo){
        log_error(logger, "No se pudo abrir archivo de pseudocódigo para PID %d", pid);
        return;
    }

    Proceso* nuevo = malloc(sizeof(Proceso));
    nuevo->pid = pid;
    nuevo->instrucciones = list_create();

    char* linea = NULL;
    size_t len = 0;
    while (getline(&linea, &len, archivo) != -1){
        linea[strcspn(linea, "\n")] = 0; // saca \n
        list_add(nuevo->instrucciones, strdup(linea));
    }
    fclose(archivo);
    if (linea) free(linea);

    list_add(procesos, nuevo);
    log_info(logger, "Cargadas %d instrucciones para PID %d", list_size(nuevo->instrucciones), pid);
    }