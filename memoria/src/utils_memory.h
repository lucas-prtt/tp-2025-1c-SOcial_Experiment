#ifndef UTILS_MEMORY_H
#define UTILS_MEMORY_H

#include <stdio.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <utils/socketUtils.h>
#include <utils/enums.h>
#include <arpa/inet.h>
#include "utils/paquetes.h"
#include "atencionKernel.h"
#include "atencionCPU.h"

typedef struct
{
    t_list* socketModulo;
} ModulosConectados;

extern t_log* logger;

int crearSocketConfig(t_config* config, char opcion[]);
void* aceptarConexiones(void* socketPtr); 
void* atenderConexion(void* socketPtr);
bool es_valida_dir_fisica(int * pid, int * direccion_fisica, int * tamanio);
t_list* entradasDesdeNumeroDePagina(int numeroPagina);
int* int_de(int valor);
void suspenderProceso(int pid);
int dessuspenderProceso(int pid);
void compactarSwap();
int obtenerFinDeSwap();
void liberarEspacioSwap(int pid);
void mergearEspaciosLibres();
bool compararEntradasSwap(void* a, void* b);
bool compararEspaciosPorInicio(void* a, void* b);

FILE * abrirSwapFile();
void borrarSwapfile();

typedef struct {
    uint32_t pid;
    t_list* instrucciones;
} t_instrucciones_por_pid;

extern t_list* lista_instrucciones;
extern pthread_mutex_t mutex_lista_instrucciones;

#endif