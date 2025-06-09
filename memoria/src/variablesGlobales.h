#ifndef VARIABLES_GLOBALES_H
#define VARIABLES_GLOBALES_H
#include "commons/collections/list.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct MetricasMemoria{
    int accesosATP;
    int instruccionesSolicitadas;
    int bajadasASwap;
    int subidasAMP;
    int lecturasDeMemoria;
    int escriturasDeMemoria;
} Metricas;
typedef struct PIDyTablaDePaginas{
    int PID;
    void** TP;
    Metricas stats; // Tambien le adjunto las metricas
    int TamMaxProceso;
} PIDyTP;

extern int* PIDPorMarco;
extern void* memoriaDeUsuario;
extern t_list* tablaDeProcesos; // Llena de PIDyTablaDePaginas
extern int maximoEntradasTabla;
extern int nivelesTablas;
extern int tamañoMarcos;
extern int tamañoMemoriaDeUsuario;
extern int numeroDeMarcos;



// De Procesos
void agregarProcesoATabla(int nuevoPID, int tamañoMaximo);
void eliminarProcesoDeTabla(int PIDEliminado);

// De Paginas
void asignarMarcoAPaginaConPIDyEntradas(int PID, t_list * entradas, int marco);
int obtenerMarcoDePaginaConPIDYEntradas(int PID, t_list * entradas);
void removerPaginaDeMarco(int marco);

// De Marcos
void * punteroAMarco(int numeroDeMarco);
int marcosDisponibles();
bool hayEspacio(int tamañoRequerido);
int PIDdelMarco(int Marco);

// De Metricas
Metricas getMetricasPorPID(int PID);
void aumentarMetricaSubidasAMemoriaPrincipal(int PID);
void aumentarMetricaLecturaDeMemoria(int PID);
void aumentarMetricaInstruccinoesSolicitadas(int PID);
void aumentarMetricaEscrituraDeMemoria(int PID);
void aumentarMetricaBajadasASwap(int PID);
void aumentarMetricaAccesoATablaDePaginas(int PID);

// Otras
void inicializarVariablesGlobales(int sizeTabla, int qNiveles, int sizeMemoria, int SizeMarcos);
void liberarVariablesGlobalesEnHeap();



#endif