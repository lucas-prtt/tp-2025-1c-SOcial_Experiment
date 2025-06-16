#ifndef TRADUCCIONES_H
#define TRADUCCIONES_H

#include <commons/collections/list.h>
#include <utils/threads.h>
#include <string.h>
#include "utils/logConfig.h"
#include <math.h>


extern int CACHE_SIZE;

extern int TLB_SIZE;
extern int cantidad_niveles_tabla_paginas;
extern int cantidad_entradas_tabla;
extern int tamanio_pagina;

typedef struct {
    int pid;
    int pagina;
    void *contenido;
    int bit_uso;
    // int bit_modificado;
    // int ultimo_uso;
} r_CACHE;

typedef struct {
    int habilitada;
    int algoritmo;
    // int tamanio;
    r_CACHE *entradas; // Revisar: r_CACHE* entradas;
} CACHE;

typedef struct {
    int pid;
    int pagina;
    int marco;
    int validez; // Para saber si el regitro de la tlb está vacia
    int ultimo_uso;
} r_TLB;

typedef struct {
    int habilitada;
    int proximo; // FIFO
    int contador_uso; // LRU
    int algoritmo;
    // int tamanio;
    r_TLB *entradas; // Revisar: r_TLB* entradas;
} TLB;

enum TIPO_ALGORITMO_REEMPLAZO {
    ALG_FIFO,
    ALG_LRU,
    ALG_CLOCK,
    ALG_CLOCK_M,
    ERROR_NO_ALG,
};


void inicializarVariablesGlobales(int socket_memoria, int cant_niveles_t, int cant_entradas_t, int tam_pag);

int traducirDeLogicaAFisica(int direccion_logica);

int inicializarCACHE(CACHE *cache);
int buscarPaginaCACHE(CACHE *cache, int pid, int nro_pagina);
int buscarIndicePaginaCACHE(CACHE *cache, int pid, int nro_pagina);

int inicializarTLB(TLB *tlb);
int buscarPaginaTLB(TLB *tlb, int pid, int nro_pagina);
int buscarIndicePaginaTLB(TLB *tlb, int pid, int nro_pagina);
void reemplazarEnTLB(TLB *tlb, int pid, int nro_pagina, int marco);
void insertarPaginaTLB(TLB *tlb, int pid, int indice_victima, int nro_pagina, int marco);
bool hayEntradaVaciaTLB(TLB *tlb, int *indice_victima);
int seleccionarEntradaVictima(TLB *tlb);
void limpiarEntradaTLB(TLB *tlb, int indice_victima);
void limpiarProcesoTLB(TLB *tlb, int pid);

enum TIPO_ALGORITMO_REEMPLAZO algoritmo_string_to_enum(char *nombreAlgoritmo);

int getNumeroPagina(int direccion_logica);
int getEntradaNivelX(int nro_pagina, int nro_nivel);
int getDesplazamiento(int direccion_logica);

#endif