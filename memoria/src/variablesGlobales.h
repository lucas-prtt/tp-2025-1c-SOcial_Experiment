#ifndef VARIABLES_GLOBALES_H
#define VARIABLES_GLOBALES_H
#include "commons/collections/list.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "utils/logConfig.h"
#include "math.h"

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
    t_list * instrucciones;
    int enSwap;
} PIDInfo;
// Todo lo relacionado con SWAP
typedef struct {
    int pid;
    int nro_pagina;
    int offset;
} EntradaSwap;
// Tabla de la informacion que está en SWAP
extern t_list* tablaSwap;
typedef struct {
    int punto_incio; // inicial del hueco
    int paginasLibres;
} EspacioLibre;

extern t_list* espaciosLibresSwapentrePaginas; // Lista de huecos entre paginas sin contar el espacio del final despues de la ultima pagina

extern int paginasLibresTotalesSwapEntreProcesos;

//@brief Vector:  PIDPorMarco[numeroDeMarco] = PID o -1 (vacio)
extern int* PIDPorMarco;
//@brief El puntero a void donde se almacena la memoria que va usar el usuario durante la ejecucion de procesos
extern void* memoriaDeUsuario;
//@brief Mutex a usar cada vez que se quiere operar sobre la memoria de usuario
extern pthread_mutex_t MUTEX_MemoriaDeUsuario;
//@brief Lista con informacion util de cada proceso: PID, puntero a TP, metricas, tamaño maximo en memoria y lista de instrucciones
extern t_list* tablaDeProcesos; 
//@brief Cuantas entradas tiene cada tabla del esquema de paginacion multinivel
extern int maximoEntradasTabla;
//@brief Cuantos niveles tiene el esquema de paginacion multinivel
extern int nivelesTablas;
//@brief El tamaño maximo de cada marco/pagina individual
extern int tamañoMarcos;
//@brief El tamaño maximo en Bytes de la memoria de usuario
extern int tamañoMemoriaDeUsuario;
//@brief Maxima cantidad de marcos admitidos en memoria
extern int numeroDeMarcos;
//@brief El directorio de donde se obtiene el pseudocodigo
extern char * directorioPseudocodigo;
//@brief El directorio donde se realizan los DUMPs de memoria
extern char * directorioDump;
//@brief Retraso para acceder a memoria en ms
extern int retrasoAcceso;
//@brief Retraso para realizar un SWAP en ms
extern int retrasoSWAP;
//@brief El directorio donde se ubica el swapfile. Incluye el nombre del archivo
extern char * directorioSwap;
// De Procesos

void agregarProcesoATabla(int nuevoPID, int tamañoMaximo);
void eliminarProcesoDeTabla(int PIDEliminado);
int tamañoProceso(int PID);
// De Paginas

void asignarMarcoAPaginaConPIDyEntradas(int PID, t_list * entradas, int marco);
int obtenerMarcoDePaginaConPIDYEntradas(int PID, t_list * entradas);
void removerPaginaDeMarco(int marco);
void vaciarTablaDePaginasDePID(int PID);
void eliminarProcesoDePIDPorMarco(int PID);
int asignarSiguienteMarcoLibreDadasLasEntradas(int PID, t_list * entradas);
int esPaginaValida(int PID, t_list * entradas);
int cantidadDePaginasDelProceso(int PID);
int cantidadDeMarcosParaAlmacenar(int tamaño);

// De Marcos

void * punteroAMarcoPorNumeroDeMarco(int numeroDeMarco);
void * punteroAMarcoPorDireccionFisica(int direccionFisica);
int direccionFisicaMarco(int numeroDeMarco);
int marcosDisponibles();
bool hayEspacio(int tamañoRequerido);
int PIDdelMarco(int Marco);
t_list * marcosDelPid(int PID);
int siguienteMarcoLibre();

void setEnSwap(int PID);
void setEnMemoria(int PID);
int estaCargado(PIDInfo * Proceso);
int estaCargadoPid(int Proceso);


// De Metricas

Metricas getMetricasPorPID(int PID);
void aumentarMetricaSubidasAMemoriaPrincipal(int PID);
void aumentarMetricaLecturaDeMemoria(int PID);
void aumentarMetricaInstruccinoesSolicitadas(int PID);
void aumentarMetricaEscrituraDeMemoria(int PID);
void aumentarMetricaBajadasASwap(int PID);
void aumentarMetricaAccesoATablaDePaginas(int PID);
void aumentarMetricaAccesoATablaDePaginasPorNiveles(int PID);
// Otras

void inicializarVariablesGlobales(int sizeTabla, int qNiveles, int sizeMemoria, int SizeMarcos, char * directorioPseudocodigo, char * directorioDump, int retrasoAcceso, int retrasoSWAP, char * PathSwap);
void liberarVariablesGlobalesEnHeap();
void agregarInstruccionesAPID(int PID, t_list * instruccionesNuevas);
t_list * obtenerInstruccionesPorPID(int PID);
char * leerInstruccion(int PID, int PC);
void simularRetrasoSWAP();
void simularRetrasoUnSoloNivel();
void simularRetrasoMultinivel();
#endif