#include "utils_kernel.h"


conexionesAModulos conexiones;

void * esperarCPUDispatch(void * socket){
    conexiones.CPUsDispatch = list_create();
    t_list * createdThreads = list_create();
    pthread_cleanup_push(closeTreadsFromListAndCleanUpList, createdThreads);
    while(1){
        int nuevoSocket;
        nuevoSocket = accept(*(int*)socket, NULL, NULL);
        if(nuevoSocket == -1) {
            pthread_testcancel(); // Para asegurarse que el -1 que sale de shutdown no entre en la lista 
        }
        IDySocket * CPUIDySocket = malloc(sizeof(IDySocket));
        CPUIDySocket->SOCKET = nuevoSocket;
        CPUIDySocket->ID = -1; // -1 = No se recibio Handshake: Se desconoce el ID del CPU
        list_add(conexiones.CPUsDispatch, CPUIDySocket);
        pthread_t * hilo = malloc(sizeof(pthread_t));
        pthread_create(hilo, NULL, handshakeCPUDispatch, CPUIDySocket);
        pthread_detach(*hilo);
        list_add(createdThreads, hilo);
        printf("- CPU conectada para dispatch\n"); //Porahi conviene poner esto en el handshake
        fflush(stdout);
    }
    pthread_cleanup_pop(1);
}

void * esperarCPUInterrupt(void * socket){
    conexiones.CPUsInterrupt = list_create();
    t_list * createdThreads = list_create();
    pthread_cleanup_push(closeTreadsFromListAndCleanUpList, createdThreads);
    while(1){
        int nuevoSocket;
        nuevoSocket = accept(*(int*)socket, NULL, NULL);
        if(nuevoSocket == -1) {
            pthread_testcancel();
        }
        IDySocket * CPUIDySocket = malloc(sizeof(IDySocket));
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

void * esperarIOEscucha(void * socket){
    conexiones.IOEscucha = list_create();
    t_list * createdThreads = list_create();
    pthread_cleanup_push(closeTreadsFromListAndCleanUpList, createdThreads);
    while(1){
        int nuevoSocket;
        nuevoSocket = accept(*(int*)socket, NULL, NULL);
        if(nuevoSocket == -1){
            pthread_testcancel();
        }
        IDySocket * IOIDYSocket = malloc(sizeof(IDySocket));
        IOIDYSocket->SOCKET = nuevoSocket;
        IOIDYSocket->ID = -1;
        list_add(conexiones.IOEscucha, IOIDYSocket) ;
        pthread_t * hilo = malloc(sizeof(pthread_t));
        pthread_create(hilo, NULL, handshakeIO, IOIDYSocket);
        pthread_detach(*hilo);
        list_add(createdThreads, hilo);
        printf("- IO conectado\n");
        fflush(stdout);
    }
    pthread_cleanup_pop(1);
    
}

int verificarModuloMemoriaDisponible(void){
    return estaConexionDisponible(conexiones.ipYPuertoMemoria.IP, conexiones.ipYPuertoMemoria.puerto);
}

void eliminarConexiones(void){ // libera los sockets de CPU e IO y borra las listas de CPU
    void liberarConexionIDYSOCKET(void * ids){
    close((*(IDySocket*)ids).SOCKET);}
    list_iterate(conexiones.CPUsDispatch, liberarConexionIDYSOCKET);
    list_iterate(conexiones.CPUsInterrupt, liberarConexionIDYSOCKET);
    list_iterate(conexiones.IOEscucha, liberarConexionIDYSOCKET);
    list_destroy_and_destroy_elements(conexiones.CPUsDispatch, free);
    list_destroy_and_destroy_elements(conexiones.CPUsInterrupt, free);
    list_destroy_and_destroy_elements(conexiones.IOEscucha, free);
    return;
}

int crearSocketDesdeConfig(t_config * config, char opcion[]){
    int puertoConfig = config_get_int_value(config, opcion);
    char puerto[7];
    sprintf(puerto, "%d", puertoConfig);
    return crearSocketServer(puerto);
}

void * handshakeCPUDispatch(void * CPUSocketEId){
    int id;
    int elSocket = ((IDySocket*)CPUSocketEId)->SOCKET;
    t_list * lista_contenido = recibir_paquete_lista(elSocket, MSG_WAITALL, NULL);    
    if(lista_contenido == NULL || list_size(lista_contenido) < 2) {
        list_destroy(lista_contenido);
        pthread_exit(NULL);
    }
    id = *(int*)list_get(lista_contenido, 1);
    ((IDySocket*)CPUSocketEId)->ID = id;
    t_paquete *paquete_resp = crear_paquete(HANDSHAKE);
    agregar_a_paquete(paquete_resp, &id, sizeof(id));
    enviar_paquete(paquete_resp, elSocket);
    eliminar_paquete(paquete_resp);
    list_destroy(lista_contenido);
    pthread_exit(NULL);
}

void * handshakeCPUInterrupt(void * CPUSocketEId){
    int id;
    int elSocket = ((IDySocket*)CPUSocketEId)->SOCKET;
    t_list * lista_contenido = recibir_paquete_lista(elSocket, MSG_WAITALL, NULL);    
    if(lista_contenido == NULL || list_size(lista_contenido) < 2) { 
        list_destroy(lista_contenido);
        pthread_exit(NULL);
    }
    id = *(int*)list_get(lista_contenido, 1);
    ((IDySocket*)CPUSocketEId)->ID = id;
    t_paquete *paquete_resp = crear_paquete(HANDSHAKE);
    agregar_a_paquete(paquete_resp, &id, sizeof(id));
    enviar_paquete(paquete_resp, elSocket);
    eliminar_paquete(paquete_resp);
    list_destroy(lista_contenido);
    pthread_exit(NULL);
}


void *handshakeIO(void *ioSocketEId) { 
    int elSocket = ((IDySocket*)ioSocketEId)->SOCKET;

    t_list * lista_contenido = recibir_paquete_lista(elSocket, MSG_WAITALL, NULL);    
    if(lista_contenido == NULL || list_size(lista_contenido) < 2) {
        list_destroy(lista_contenido);
        pthread_exit(NULL);
    }
    int id = *(int*)list_get(lista_contenido, 1);
    ((IDySocket*)ioSocketEId)->ID = id;
    t_paquete *paquete_resp_io = crear_paquete(HANDSHAKE);
    agregar_a_paquete(paquete_resp_io, &id, sizeof(id));
    enviar_paquete(paquete_resp_io, elSocket);

    eliminar_paquete(paquete_resp_io);
    list_destroy(lista_contenido);
    
    pthread_exit(NULL);
}
