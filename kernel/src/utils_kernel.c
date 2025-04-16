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
        int * socketConexion = malloc(sizeof(int));
        *socketConexion = nuevoSocket;
        list_add(conexiones.CPUsDispatch, socketConexion);
        printf("- CPU conectada para dispatch\n");
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
        int * socketConexion = malloc(sizeof(int));
        *socketConexion = nuevoSocket;
        list_add(conexiones.CPUsInterrupt, socketConexion);
        printf("- CPU conectada para Interrupt\n");
        fflush(stdout);
    }
}

void * esperarIOEscucha(void * socket){
    conexiones.IOEscucha = -1;
    while(1){
        int nuevoSocket;
        nuevoSocket = accept(*(int*)socket, NULL, NULL);
        if(nuevoSocket == -1) {
            pthread_testcancel();
        }
        conexiones.IOEscucha = nuevoSocket;
        printf("- IO conectado\n");
        fflush(stdout);
    }
}

int verificarModuloMemoriaDisponible(void){
    return estaConexionDisponible(conexiones.ipYPuertoMemoria.IP, conexiones.ipYPuertoMemoria.puerto);
}

void eliminarConexiones(void){ // libera los sockets de CPU e IO y borra las listas de CPU
    list_iterate(conexiones.CPUsDispatch, liberarConexionPuntero);
    list_iterate(conexiones.CPUsInterrupt, liberarConexionPuntero);
    list_destroy_and_destroy_elements(conexiones.CPUsDispatch, free);
    list_destroy_and_destroy_elements(conexiones.CPUsInterrupt, free);
    liberarConexion(conexiones.IOEscucha);
    return;
}

int crearSocketDesdeConfig(t_config * config, char opcion[]){
    int puertoConfig = config_get_int_value(config, opcion);
    char puerto[7];
    sprintf(puerto, "%d", puertoConfig);
    return crearSocketServer(puerto);
}

