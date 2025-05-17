#ifndef PROCESOS_H
#define PROCESOS_H
#include "utils_procesos.h"
#include <semaphore.h>
#include <pthread.h>
#include "utils_kernel.h"
#include "utils/socketUtils.h"
#include "utils/logConfig.h"
#include "utils/socketUtils.h"
#include "utils/tiempo.h"

typedef struct{
    char * nombre;
    sem_t sem_peticiones;
    t_list * cola;
} PeticionesIO;

typedef struct {
    int PID;
    int milisegundos;
} Peticion;

extern t_list * listasProcesos[7]; // Vector de lista para guardar procesos


void procesos_c_inicializarVariables(); // Necesaria para inicializar listas y semaforos globales


#endif