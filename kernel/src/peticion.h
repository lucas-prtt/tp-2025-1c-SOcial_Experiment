#ifndef PETICION_H
#define PETICION_H
#include <semaphore.h>
typedef struct{
    int PID;
    int milisegundos;
    sem_t sem_estado; // = 1
    int estado; // = PETICION_BLOQUEADA
} Peticion;
#endif