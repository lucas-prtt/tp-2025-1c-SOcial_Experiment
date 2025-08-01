#include "utils_kernel.h"


conexionesAModulos conexiones;
sem_t evaluarFinKernel;
t_list * hilos;
void * esperarCPUDispatch(void * socket) {
    conexiones.CPUsDispatch = list_create();
    t_list * createdThreads = list_create();
    pthread_cleanup_push(joinTreadsFromListAndCleanUpList, createdThreads);
    while(1) {
        int nuevoSocket;
        nuevoSocket = accept(*(int*)socket, NULL, NULL);
        if(nuevoSocket == -1) {
            pthread_exit(NULL); // Para asegurarse que el -1 que sale de shutdown no entre en la lista 
        }
        IDySocket_CPU *CPUIDySocket = malloc(sizeof(IDySocket_CPU));
        CPUIDySocket->SOCKET = nuevoSocket;
        CPUIDySocket->ID = -1; // -1 = No se recibio Handshake: Se desconoce el ID del CPU
        list_add(conexiones.CPUsDispatch, CPUIDySocket);
        pthread_t *hilo = malloc(sizeof(pthread_t));
        pthread_create(hilo, NULL, handshakeCPUDispatch, CPUIDySocket);
        list_add(createdThreads, hilo);
        printf("- CPU D\n"); //Por ahi conviene poner esto en el handshake
        fflush(stdout);
    }
    pthread_cleanup_pop(1);
}

IDySocket_CPU * buscarCPUInterruptPorID(int id){
    IDySocket_CPU * cpu = NULL;
    #ifndef __INTELLISENSE__
    bool coinicide (void * elem){
        return ((IDySocket_CPU*)elem)->ID == id;
    }
    cpu = list_find(conexiones.CPUsInterrupt, coinicide);
    #endif
    return cpu;
}

void * esperarCPUInterrupt(void * socket) {
    conexiones.CPUsInterrupt = list_create();
    t_list * createdThreads = list_create();
    pthread_cleanup_push(joinTreadsFromListAndCleanUpList, createdThreads);
    while(1) {
        int nuevoSocket;
        nuevoSocket = accept(*(int*)socket, NULL, NULL);
        if(nuevoSocket == -1) {
            pthread_exit(NULL);
        }
        IDySocket_CPU * CPUIDySocket = malloc(sizeof(IDySocket_CPU));
        CPUIDySocket->SOCKET = nuevoSocket;
        CPUIDySocket->ID = -1;
        list_add(conexiones.CPUsInterrupt, CPUIDySocket);
        pthread_t * hilo = malloc(sizeof(pthread_t));
        pthread_create(hilo, NULL, handshakeCPUInterrupt, CPUIDySocket);
        list_add(createdThreads, hilo);
        printf("- CPU I\n");
        fflush(stdout);
    }
    pthread_cleanup_pop(1);
}

void * esperarIOEscucha(void * socket) {
    conexiones.IOEscucha = list_create();
    t_list * createdThreads = list_create();
    pthread_cleanup_push(joinTreadsFromListAndCleanUpList, createdThreads);
    while(1) {
        int nuevoSocket;
        nuevoSocket = accept(*(int*)socket, NULL, NULL);
        if(nuevoSocket == -1) {
            pthread_exit(NULL);
        }
        NombreySocket_IO * IONombreYSocket = malloc(sizeof(NombreySocket_IO));
        IONombreYSocket->SOCKET = nuevoSocket;
        list_add(conexiones.IOEscucha, IONombreYSocket) ;
        pthread_t * hilo = malloc(sizeof(pthread_t));
        pthread_create(hilo, NULL, handshakeIO, IONombreYSocket);
        list_add(createdThreads, hilo);
        printf("- IO \n");
        fflush(stdout);
    }
    pthread_cleanup_pop(1);
}

int verificarModuloMemoriaDisponible(void) {
    int socketMemoria = conectarSocketClient(conexiones.ipYPuertoMemoria.IP, conexiones.ipYPuertoMemoria.puerto);
    int error = 0;
    if (socketMemoria == -1){
        log_trace(logger, "verificarModuloMemoriaDisponible tiene socket = %d", socketMemoria);
        error = 1;
    }else{
        error = handshakeMemoria(socketMemoria);
        log_debug(logger, "Resultado de handshake de memoria: %d", error);
        t_paquete * respuesta = crear_paquete(VERIFICARCONEXION);
        enviar_paquete(respuesta, socketMemoria);
        eliminar_paquete(respuesta);
        liberarConexion(socketMemoria);
    }
    return error;
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
    log_debug(logger, "Dispatch Handshake - ID:%d, Socket: %d", ((IDySocket_CPU*)CPUSocketEId)->ID, ((IDySocket_CPU*)CPUSocketEId)->SOCKET);
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
    log_debug(logger, "Interrupt Handshake - ID:%d, Socket: %d", ((IDySocket_CPU*)CPUSocketEId)->ID, ((IDySocket_CPU*)CPUSocketEId)->SOCKET);
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
    log_debug(logger, "IO Handshake - NOMBRE: %s, Socket: %d", ((NombreySocket_IO*)ioSocketYNombre)->NOMBRE, ((NombreySocket_IO*)ioSocketYNombre)->SOCKET);
    eliminar_paquete(paquete_resp_io);
    eliminar_paquete_lista(lista_contenido);

    pthread_t * nuevoHilo;
    nuevoHilo = malloc(sizeof(pthread_t));
    pthread_create(nuevoHilo, NULL, IOThread, ioSocketYNombre);
    list_add(hilos, nuevoHilo);
    log_debug(logger, "   -Se inicio el hilo de IO: %ld", * nuevoHilo);
    
    pthread_exit(NULL);
}

void cerrarKernel() {
    cerrarConfigYLog();
    abort();
}

void generarHilos(t_list * hilos, int cantidad, void * func(void *), t_list * parametros){
    pthread_t * nuevoHilo;
    log_debug(logger, "Inicio de generacion de hilos");
    for (int i = 0; i < cantidad; i++) { // i no esta inicializado => No generaba los hilos
        nuevoHilo = malloc(sizeof(pthread_t));
        if (parametros == NULL)
        pthread_create(nuevoHilo, NULL, func, NULL);
        else
        pthread_create(nuevoHilo, NULL, func, list_get(parametros, i));
        list_add(hilos, nuevoHilo);
        log_debug(logger, "   -Se inicio el hilo: %ld", * nuevoHilo);
    }
}
void eliminarHilos(t_list * hilos){
    pthread_t * hiloPorMorir;
    log_debug(logger, "Inicio de eliminacion de hilos");
    for (int i = 0; i < list_size(hilos); i++) {
        hiloPorMorir = (pthread_t*) list_get(hilos, i);
        log_debug(logger, "   -Se eliminará el hilo: %ld", * hiloPorMorir);
        pthread_detach(*hiloPorMorir);
        pthread_cancel(*hiloPorMorir);
    }
    log_debug(logger, "Terminamos de eliminar hilos");
}


int handshakeMemoria(int socketMemoria){
    t_paquete * saludo = crear_paquete(SOYKERNEL);
    log_debug(logger, "Enviando hadshake a memoria en socket: %d", socketMemoria);
    int rta;
    enviar_paquete(saludo, socketMemoria);
    log_trace(logger, "Paquete %p enviado", saludo);
    eliminar_paquete(saludo);
    t_list * respuesta;
    respuesta = recibir_paquete_lista(socketMemoria, MSG_WAITALL, &rta);
    log_trace(logger, "Paquete recibido, pointer = %p", respuesta);
    if(respuesta == NULL || rta != SOYMEMORIA){
        eliminar_paquete_lista(respuesta);
        log_error(logger, "Error en el handshake con memoria: respuesta = %d", rta);
        return -1;
    }
    else{
            log_debug(logger, "Handshake valido");
        eliminar_paquete_lista(respuesta);
        return 0;
    }
}
