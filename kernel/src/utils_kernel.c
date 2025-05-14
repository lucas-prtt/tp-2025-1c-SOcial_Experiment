#include "utils_kernel.h"


conexionesAModulos conexiones;
t_log * logger;
t_config * config;

void * esperarCPUDispatch(void * socket) {
    conexiones.CPUsDispatch = list_create();
    t_list * createdThreads = list_create();
    pthread_cleanup_push(closeTreadsFromListAndCleanUpList, createdThreads);
    while(1) {
        int nuevoSocket;
        nuevoSocket = accept(*(int*)socket, NULL, NULL);
        if(nuevoSocket == -1) {
            pthread_testcancel(); // Para asegurarse que el -1 que sale de shutdown no entre en la lista 
        }
        IDySocket_CPU *CPUIDySocket = malloc(sizeof(IDySocket_CPU));
        CPUIDySocket->SOCKET = nuevoSocket;
        CPUIDySocket->ID = -1; // -1 = No se recibio Handshake: Se desconoce el ID del CPU
        list_add(conexiones.CPUsDispatch, CPUIDySocket);
        pthread_t *hilo = malloc(sizeof(pthread_t)); //
        pthread_create(hilo, NULL, handshakeCPUDispatch, CPUIDySocket);
        pthread_detach(*hilo);
        list_add(createdThreads, hilo);
        printf("- CPU conectada para dispatch\n"); //Por ahi conviene poner esto en el handshake
        fflush(stdout);
    }
    pthread_cleanup_pop(1);
}

void * esperarCPUInterrupt(void * socket) {
    conexiones.CPUsInterrupt = list_create();
    t_list * createdThreads = list_create();
    pthread_cleanup_push(closeTreadsFromListAndCleanUpList, createdThreads);
    while(1) {
        int nuevoSocket;
        nuevoSocket = accept(*(int*)socket, NULL, NULL);
        if(nuevoSocket == -1) {
            pthread_testcancel();
        }
        IDySocket_CPU * CPUIDySocket = malloc(sizeof(IDySocket_CPU));
        CPUIDySocket->SOCKET = nuevoSocket;
        CPUIDySocket->ID = -1;
        list_add(conexiones.CPUsInterrupt, CPUIDySocket);
        pthread_t * hilo = malloc(sizeof(pthread_t));
        pthread_create(hilo, NULL, handshakeCPUInterrupt, CPUIDySocket);
        pthread_detach(*hilo);
        list_add(createdThreads, hilo);
        printf("- CPU conectada para Interrupt\n");
        fflush(stdout);
    }
    pthread_cleanup_pop(1);
}

void * esperarIOEscucha(void * socket) {
    conexiones.IOEscucha = list_create();
    t_list * createdThreads = list_create();
    pthread_cleanup_push(closeTreadsFromListAndCleanUpList, createdThreads);
    while(1) {
        int nuevoSocket;
        nuevoSocket = accept(*(int*)socket, NULL, NULL);
        if(nuevoSocket == -1) {
            pthread_testcancel();
        }
        NombreySocket_IO * IONombreYSocket = malloc(sizeof(NombreySocket_IO));
        IONombreYSocket->SOCKET = nuevoSocket;
        list_add(conexiones.IOEscucha, IONombreYSocket) ;
        pthread_t * hilo = malloc(sizeof(pthread_t));
        pthread_create(hilo, NULL, handshakeIO, IONombreYSocket);
        pthread_detach(*hilo);
        list_add(createdThreads, hilo);
        printf("- IO conectado\n");
        fflush(stdout);
    }
    pthread_cleanup_pop(1);
}

int verificarModuloMemoriaDisponible(void) {
    return estaConexionDisponible(conexiones.ipYPuertoMemoria.IP, conexiones.ipYPuertoMemoria.puerto);
}

void liberarConexion_IDYSOCKET_CPU(void * ids) {
    close((*(IDySocket_CPU*)ids).SOCKET);
}

void liberarConexion_NOMBREYSOCKET_IO(void * ids) {
    close((*(NombreySocket_IO*)ids).SOCKET);
}

void eliminarConexiones(void) {
    list_iterate(conexiones.CPUsDispatch, liberarConexion_IDYSOCKET_CPU);
    list_iterate(conexiones.CPUsInterrupt, liberarConexion_IDYSOCKET_CPU);
    list_iterate(conexiones.IOEscucha, liberarConexion_NOMBREYSOCKET_IO);
    list_destroy_and_destroy_elements(conexiones.CPUsDispatch, free);
    list_destroy_and_destroy_elements(conexiones.CPUsInterrupt, free);
    list_destroy_and_destroy_elements(conexiones.IOEscucha, free);
    return;
}

int crearSocketDesdeConfig(t_config * config, char opcion[]) {
    int puertoConfig = config_get_int_value(config, opcion);
    char puerto[7];
    sprintf(puerto, "%d", puertoConfig);
    return crearSocketServer(puerto);
}

void *handshakeCPUDispatch(void *CPUSocketEId) {
    int socket_CPU_Dispatch = ((IDySocket_CPU*)CPUSocketEId)->SOCKET;
    t_list *lista_contenido = recibir_paquete_lista(socket_CPU_Dispatch, MSG_WAITALL, NULL);
    if(lista_contenido == NULL || list_size(lista_contenido) < 2) {
        enviar_paquete_error(socket_CPU_Dispatch, lista_contenido);
        pthread_exit(NULL);
    }
    int id = *(int*)list_get(lista_contenido, 1);
    ((IDySocket_CPU*)CPUSocketEId)->ID = id;
    t_paquete *paquete_resp_cpu = crear_paquete(HANDSHAKE);
    agregar_a_paquete(paquete_resp_cpu, &id, sizeof(id));
    enviar_paquete(paquete_resp_cpu, socket_CPU_Dispatch);

    eliminar_paquete(paquete_resp_cpu);
    eliminar_paquete_lista(lista_contenido);
    pthread_exit(NULL);
}

void *handshakeCPUInterrupt(void *CPUSocketEId) { 
    int socket_CPU_Interrupt  = ((IDySocket_CPU*)CPUSocketEId)->SOCKET;
    t_list *lista_contenido = recibir_paquete_lista(socket_CPU_Interrupt, MSG_WAITALL, NULL);
    if(lista_contenido == NULL || list_size(lista_contenido) < 2) {
        enviar_paquete_error(socket_CPU_Interrupt, lista_contenido);
        pthread_exit(NULL);
    }
    int id = *(int*)list_get(lista_contenido, 1);
    ((IDySocket_CPU*)CPUSocketEId)->ID = id;
    t_paquete *paquete_resp_cpu = crear_paquete(HANDSHAKE);
    agregar_a_paquete(paquete_resp_cpu, &id, sizeof(id));
    enviar_paquete(paquete_resp_cpu, socket_CPU_Interrupt);

    eliminar_paquete(paquete_resp_cpu);
    eliminar_paquete_lista(lista_contenido);
    pthread_exit(NULL);
}

void *handshakeIO(void *ioSocketYNombre) { 
    int socket_io = ((NombreySocket_IO*)ioSocketYNombre)->SOCKET;
    t_list *lista_contenido = recibir_paquete_lista(socket_io, MSG_WAITALL, NULL);
    if(lista_contenido == NULL || list_size(lista_contenido) < 2) {
        enviar_paquete_error(socket_io, lista_contenido);
        pthread_exit(NULL);
    }
    char* nombre = (char*)list_get(lista_contenido, 1); //char[10] = {i,m,p,r,e,s,o,r,a,\0}
    int* tamaño = (int*)list_get(lista_contenido, 0); //10
    ((NombreySocket_IO*)ioSocketYNombre)->NOMBRE = malloc(*tamaño); //Alocar 10 bytes en pointer nombre del struct socket y nombre
    memcpy(((NombreySocket_IO*)ioSocketYNombre)->NOMBRE, nombre, *tamaño); //Copiar el nombre (funcionaria capaz strcpy tambien?)
    t_paquete *paquete_resp_io = crear_paquete(HANDSHAKE);
    agregar_a_paquete(paquete_resp_io, nombre, strlen(nombre) + 1);
    enviar_paquete(paquete_resp_io, socket_io);
    log_debug(logger, "RECIBIDO HANDSHAKE: %s", ((NombreySocket_IO*)ioSocketYNombre)->NOMBRE);
    eliminar_paquete(paquete_resp_io);
    eliminar_paquete_lista(lista_contenido);
    pthread_exit(NULL);
}

void cerrarKernel() {
    cerrarConfigYLog();
    abort();
}
