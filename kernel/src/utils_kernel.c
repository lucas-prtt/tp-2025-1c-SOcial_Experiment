#include "utils_kernel.h"


conexionesAModulos conexiones;

void * esperarCPUDispatch(void * socket){
    conexiones.CPUsDispatch = list_create();
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
        pthread_t hilo;
        pthread_create(&hilo, NULL, handshakeCPUDispatch, socket);
        pthread_detach(hilo);
        printf("- CPU conectada para dispatch\n"); //Porahi conviene poner esto en el handshake
        fflush(stdout);
    }
}

void * esperarCPUInterrupt(void * socket){
    conexiones.CPUsInterrupt = list_create();
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
        pthread_t hilo;
        pthread_create(&hilo, NULL, handshakeCPUInterrupt, socket);
        pthread_detach(hilo);
        printf("- CPU conectada para Interrupt\n");
        fflush(stdout);
    }
}

void * esperarIOEscucha(void * socket){
    conexiones.IOEscucha.ID = -1;
    conexiones.IOEscucha.SOCKET = -1;
    while(1){
        int nuevoSocket;
        nuevoSocket = accept(*(int*)socket, NULL, NULL);
        if(nuevoSocket == -1) {
            pthread_testcancel();
        }
        conexiones.IOEscucha.SOCKET = nuevoSocket;
        handshakeIO(socket);
        printf("- IO conectado\n");
        fflush(stdout);
    }
}

int verificarModuloMemoriaDisponible(void){
    return estaConexionDisponible(conexiones.ipYPuertoMemoria.IP, conexiones.ipYPuertoMemoria.puerto);
}

void eliminarConexiones(void){ // libera los sockets de CPU e IO y borra las listas de CPU
    void liberarConexionIDYSOCKET(void * ids){
    close((*(IDySocket*)ids).SOCKET);}
    list_iterate(conexiones.CPUsDispatch, liberarConexionIDYSOCKET);
    list_iterate(conexiones.CPUsInterrupt, liberarConexionIDYSOCKET);
    list_destroy_and_destroy_elements(conexiones.CPUsDispatch, free);
    list_destroy_and_destroy_elements(conexiones.CPUsInterrupt, free);
    liberarConexion(conexiones.IOEscucha.SOCKET);
    return;
}

int crearSocketDesdeConfig(t_config * config, char opcion[]){
    int puertoConfig = config_get_int_value(config, opcion);
    char puerto[7];
    sprintf(puerto, "%d", puertoConfig);
    return crearSocketServer(puerto);
}

void * handshakeCPUDispatch(void * socket){
    bool socket_coincide(void * cpu){
        return *(int*)socket == (*(IDySocket*)cpu).SOCKET;
    }// Closure que se fija si el socket de la lista coincide con el del thread
    sleep(5); // Doy tiempo a que CPU mande el handshake
    int * id = malloc(sizeof(int));
    int err = recv(*(int*)socket, id, sizeof(id), MSG_DONTWAIT); // Se fija si mando el handshake. Si no lo mando no se bloquea
    if (err == -1){free(id);pthread_exit(NULL);}
    void * infoCPU = list_find(conexiones.CPUsDispatch, socket_coincide);
    if(infoCPU != NULL){
        (*(IDySocket*)infoCPU).ID = *id;
    }
    free(id);
    pthread_exit(NULL);
}
void * handshakeCPUInterrupt(void * socket){
    bool socket_coincide(void * cpu){
        return *(int*)socket == (*(IDySocket*)cpu).SOCKET;
    }// Closure que se fija si el socket de la lista coincide con el del thread
    sleep(5); // Doy tiempo a que CPU mande el handshake
    int * id = malloc(sizeof(int));
    int err = recv(*(int*)socket, id, sizeof(id), MSG_DONTWAIT); // Se fija si mando el handshake. Si no lo mando no se bloquea
    if (err == -1){free(id);pthread_exit(NULL);}
    void * infoCPU = list_find(conexiones.CPUsInterrupt, socket_coincide);
    if(infoCPU != NULL){
        (*(IDySocket*)infoCPU).ID = *id;
    }
    free(id);
    pthread_exit(NULL);
}
void handshakeIO(void * socket){
    sleep(5); // Doy tiempo a que CPU mande el handshake
    int * id = malloc(sizeof(int));
    int err = recv(*(int*)socket, id, sizeof(id), MSG_DONTWAIT); // Se fija si mando el handshake. Si no lo mando no se bloquea
    if (err == -1){free(id);return;}
    conexiones.IOEscucha.ID = *id;
    free(id);
    return;
}

