#ifndef TRADUCCIONES_H
#define TRADUCCIONES_H

#include <commons/collections/list.h>
#include <utils/threads.h>
#include <string.h>
#include <utils/paquetes.h>
#include <sys/socket.h>
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
    int bit_modificado;
} r_CACHE;

typedef struct {
    int habilitada;
    int puntero_clock;
    int algoritmo;
    r_CACHE *entradas;
} CACHE;

typedef struct {
    int pid;
    int pagina;
    int marco;
    int validez;
    int ultimo_uso;
} r_TLB;

typedef struct {
    int habilitada;
    int proximo; //puntero
    int contador_uso;
    int algoritmo;
    r_TLB *entradas;
} TLB;

enum TIPO_ALGORITMO_REEMPLAZO {
    ALG_FIFO,
    ALG_LRU,
    ALG_CLOCK,
    ALG_CLOCK_M,
    ERROR_NO_ALG,
};



void inicializarVariablesGlobales(int socket_memoria, int cant_niveles_t, int cant_entradas_t, int tam_pag);

int buscarMarcoAMemoria(int socket_memoria, int pid, int nro_pagina);
int recibirDireccionTabla(int socket_memoria);
int recibirMarco(int socket_memoria);
void leerDatoMemoria(int socket_memoria, int pid, int direccion_fisica, int tamanio);
void escribirDatoMemoria(int socket_memoria, int pid, int direccion_fisica, char *datos);


int inicializarCACHE(CACHE *cache);
void *buscarPaginaCACHE(CACHE *cache, int pid, int nro_pagina);
int buscarIndicePaginaCACHE(CACHE *cache, int pid, int nro_pagina);





int inicializarTLB(TLB *tlb);
int buscarPaginaTLB(TLB *tlb, int pid, int nro_pagina);
int buscarIndicePaginaTLB(TLB *tlb, int pid, int nro_pagina);
void actualizarTLB(TLB *tlb, int pid, int nro_pagina, int marco);
void insertarPaginaTLB(TLB *tlb, int pid, int indice_victima, int nro_pagina, int marco);
bool hayEntradaVaciaTLB(TLB *tlb, int *indice_victima);
int seleccionarEntradaVictimaTLB(TLB *tlb);
void limpiarEntradaTLB(TLB *tlb, int indice_victima);
void limpiarProcesoTLB(TLB *tlb, int pid);

enum TIPO_ALGORITMO_REEMPLAZO algoritmo_string_to_enum(char *nombreAlgoritmo);

int getNumeroPagina(int direccion_logica);
int getEntradaNivelX(int nro_pagina, int nro_nivel);
int getDesplazamiento(int direccion_logica);

#endif