#ifndef UTILS_PROCESOS_H
#define UTILS_PROCESOS_H
#include <commons/collections/list.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils/logConfig.h"
#include <string.h>
#include "procesos.h"
#include <semaphore.h>
#include "utils/tiempo.h"
#include "utils/enums.h"
#include "peticion.h"
#include "utils_kernel.h"

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
    int ME[7]; // Cantidad de ingresos del procesos a cada estados
    int MT[7]; // Equivale a aproximadamente 25 dias en miilisegundos por cada estado. Se puede pasar a unsigned long long int para 580 millones de años, pero para esto me parece irrelevante
    
    int EST;     // Milisegundos de la estimacion actual, estimada tras rafaga de CPU
    int EJC_ANT; // Milisegundos de la ejecucion anterior, actualizada tras rafaga de CPU
    int EJC_ACT; // Milisegundos de la ejecucion actual, en caso de que se haya interrumpido a mitad de la rafaga (SRT)
    char * PATH; // Path del pseudocodigo
    int SIZE;    // Tamaño en memoria
    t_timeDifference tiempoEnEstado; // Usado para medir cuanto tiempo permanece en cada estado
                                     // Se lo inicia al crear el proceso y resetea por cada cambio de estado, sumandose el tiempo medido en el estado correspondiente
    IDySocket_CPU * ProcesadorQueLoEjecutaDispatch; // NULL si no esta en ejecucion.
                                            // Sirve para mandar mas facil los pedidos de interrupt solo conociendo el proceso 
    IDySocket_CPU * ProcesadorQueLoEjecutaInterrupt;

} t_PCB;

typedef struct{
    int PID;
    int socket;
}PIDySocket;



t_PCB * crearPCB(int id, char * path, int size);
void enviarSolicitudDumpMemory(int PID, int * socketMemoria);
void liberarMemoria(int PID);
char * syscallAsString(CODIGO_OP syscall);
void nuevoProceso(int id, char * path, int size, t_list * listaProcesos[]);
void cambiarEstado(int idProceso, enum estado estadoSiguiente, t_list * listaProcesos[]); // Sin probar
char * estadoAsString(enum estado);
enum algoritmo algoritmoStringToEnum(char * algoritmo);
void ordenar_cola_ready(t_list * listaProcesos[], enum algoritmo algoritmo);
void * procesoMasCorto(void * p1, void * p2);
int encolarPeticionIO(char * nombreIO, Peticion * peticion, t_list * lista_peticiones);
void actualizarEstimacion(t_PCB * proceso, float alfa);
t_PCB * procesoADesalojar(t_list * listasProcesos[], enum algoritmo alg); // Puede devolver NULL si no requiere desalojo
void eliminarPeticion(Peticion * pet);
Peticion * crearPeticion(int PID, int milisegundos);
// Funciones auxiliares
t_PCB * encontrarProcesoPorPIDYLista(t_list * lista, int pid);
int cambiarEstado_EstadoActualConocido(int idProceso, enum estado estadoActual, enum estado estadoSiguiente, t_list * listaProcesos[]);
void aparecioOtroProceso();
void eliminamosOtroProceso();
int getProcesosMolestando();
void enviarSolicitudSuspensionProceso(int PID);
void terminarProcesoPorPeticionInvalida(void * elem);

#endif