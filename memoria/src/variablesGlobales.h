#ifndef VARIABLES_GLOBALES_H
#define VARIABLES_GLOBALES_H
#include "commons/collections/list.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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

void removerPaginaDeMarco(int marco);
void inicializarVariablesGlobales(int sizeTabla, int qNiveles, int sizeMemoria, int SizeMarcos);
int obtenerMarcoDePaginaConPIDYEntradas(int PID, t_list * entradas);
void asignarMarcoAPaginaConPIDyEntradas(int PID, t_list * entradas, int marco);
void eliminarProcesoDeTabla(int PIDEliminado);
void agregarProcesoATabla(int nuevoPID, int tamañoMaximo);
void liberarVariablesGlobalesEnHeap();
void * punteroAMarco(int numeroDeMarco);
int marcosDisponibles();
bool hayEspacio(int tamañoRequerido);






#endif