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

int crearSocketDesdeConfig(t_config * config, char opcion[]){
    int puertoCPUDispatch = config_get_int_value(config, opcion);
    char puerto[7];
    sprintf(puerto, "%d", puertoCPUDispatch);
    return crearSocketServer(puerto);
}

