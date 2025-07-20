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
#include "utils_kernel.h"
#include <unistd.h>
#include "peticion.h"
#include <errno.h>

typedef struct{
    char * nombre;
    sem_t sem_peticiones;
    t_list * cola;
    pthread_mutex_t MUTEX_cola;
    int instancias;
} PeticionesIO;


typedef enum PeticionEstado{
    PETICION_BLOQUEADA, // No se realizo el IO y no se suspendio
    PETICION_SUSPENDIDA, // Se suspendio y no se realizo el IO
    PETICION_FINALIZADA // Se realizo el IO, el Timer no hace nada
}PeticionEstado;




extern t_list * listasProcesos[7]; // Vector de lista para guardar procesos
extern int qProcesosMolestando;     // Procesos que se deben ejecutar
extern pthread_mutex_t mutex_procesos_molestando;
void procesos_c_inicializarVariables(); // Necesaria para inicializar listas y semaforos globales
void * confirmDumpMemoryThread(void * Params);
void * IOThread(void * NOMBREYSOCKETIO);
void * ingresoAReadyThread(void * _);
void * orderThread(void * _);
void * dispatcherThread(void * IDYSOCKETDISPATCH);
void post_sem_introducirAReady(); // Para marcar el inicio desde main
void * temporizadorSuspenderThread(void * param);
#endif

