#ifndef UTILS_PROCESOS_H
#define UTILS_PROCESOS_H
#include <commons/collections/list.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils/logConfig.h"
enum estado{ // Para indicar el estado dentro del vector de ME y MT
    NEW = 0,
    READY = 1,
    EXEC = 2,
    EXIT = 3,
    BLOCKED = 4,
    SUSP_BLOCKED = 5,
    SUSP_READY = 6
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



// Funciones auxiliares
t_PCB * encontrarProcesoPorPIDYLista(t_list * lista, int pid);
void cambiarEstado_EstadoActualConocido(int idProceso, enum estado estadoActual, enum estado estadoSiguiente, t_list * listaProcesos[]);

#endif