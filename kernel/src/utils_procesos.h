#ifndef UTILS_PROCESOS_H
#define UTILS_PROCESOS_H
#include <commons/collections/list.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils/logConfig.h"
#include <string.h>



enum estado{ // Para indicar el estado dentro del vector de ME y MT
    NEW = 0,
    READY = 1,
    EXEC = 2,
    EXIT = 3,
    BLOCKED = 4,
    SUSP_BLOCKED = 5,
    SUSP_READY = 6
};

enum algoritmo{
    FIFO = 0,
    SJF = 1,
    SRT = 2,
    PMCP = 3,
    ERROR_NO_ALGORITMO = -1
};

typedef struct{
    int PID;
    int PC;
    int ME[7];
    int MT[7]; // Equivale a aproximadamente 25 dias en miilisegundos por cada estado. Se puede pasar a unsigned long long int para 580 millones de años, pero para esto me parece irrelevante
    
    int EST_ANT; // Milisegundos de la estimacion anterior
    int EST_ACT; // Milisegundos de la estimacion actual
    int EJC_ANT; // Milisegundos de la ejecucion anterior
    char * PATH;
    int SIZE;
} t_PCB;

t_PCB * crearPCB(int id, char * path, int size);



void nuevoProceso(int id, char * path, int size, t_list * listaProcesos[]);
void cambiarEstado(int idProceso, enum estado estadoSiguiente, t_list * listaProcesos[]); // Sin probar
char * estadoAsString(enum estado);
enum algoritmo algoritmoStringToEnum(char * algoritmo);
void ordenar_cola_ready(t_list * listaProcesos[], enum algoritmo algoritmo);

// Funciones auxiliares
t_PCB * encontrarProcesoPorPIDYLista(t_list * lista, int pid);
void cambiarEstado_EstadoActualConocido(int idProceso, enum estado estadoActual, enum estado estadoSiguiente, t_list * listaProcesos[]);

#endif