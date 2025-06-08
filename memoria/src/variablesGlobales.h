#ifndef VARIABLES_GLOBALES_H
#define VARIABLES_GLOBALES_H
#include "commons/collections/list.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct PIDyTablaDePaginas{
    int PID;
    void** TP;
} PIDyTP;


extern void* memoriaDeUsuario;
extern t_list* tablaDeProcesos; // Llena de PIDyTablaDePaginas
extern int maximoEntradasTabla;
extern int nivelesTablas;
extern int tamañoMarcos;
extern int tamañoMemoriaDeUsuario;

void inicializarVariablesGlobales(int sizeTabla, int qNiveles);
int obtenerMarcoDePaginaConPIDYEntradas(int PID, t_list * entradas);
void asignarMarcoAPaginaConPIDyEntradas(int PID, t_list * entradas, int marco);
void eliminarProcesoDeTabla(int PIDEliminado);
void agregarProcesoATabla(int nuevoPID);





#endif