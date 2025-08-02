#ifndef TRADUCCIONES_H
#define TRADUCCIONES_H

#include <commons/collections/list.h>
#include <utils/threads.h>
#include <string.h>
#include <utils/paquetes.h>
#include <unistd.h>
#include <sys/socket.h>
#include "utils/logConfig.h"
#include <math.h>
#include <limits.h>


extern int CACHE_SIZE;
extern int CACHE_RETARDO;
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
    int ultimo_uso;
} r_TLB;

typedef struct {
    int habilitada;
    int proximo;
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

/* forward declaration */
typedef struct cpu_t cpu_t;
void escribirEnMemoria(cpu_t *cpu, int pid, int direccion_logica, char *datos);
void leerDeMemoria(cpu_t *cpu, int pid, int direccion_logica, int tamanio);
void escribirEnCache(cpu_t *cpu, int pid, int direccion_logica, char *datos);
void leerDeCache(cpu_t *cpu, int pid, int direccion_logica, int tamanio);

void inicializarVariablesGlobales(int socket_memoria, int cant_niveles_t, int cant_entradas_t, int tam_pag);

int traducirDireccionMMU(cpu_t *cpu, int pid, int direccion_logica);
int getNumeroPagina(int direccion_logica);
int getEntradaNivelX(int nro_pagina, int nro_nivel);
int getDesplazamiento(int direccion_logica);
int traducirDireccion(int marco);
int buscarMarcoAMemoria(int socket_memoria, int pid, int nro_pagina);
void escribirPaginaCompletaEnMemoria(int socket_memoria, int pid, int direccion_fisica, char *datos);
void escribirSeccionPaginaEnMemoria(int socket_memoria, int pid, int direccion_fisica, char *datos, int tamanio_datos);
void leerPaginaCompletaMemoria(int socket_memoria, int pid, int direccion_fisica, int tamanio);
void leerSeccionPaginaMemoria(int socket_memoria, int pid, int direccion_fisica, int tamanio);

void inicializarCACHE(CACHE *cache);
void *buscarPaginaCACHE(CACHE *cache, int pid, int nro_pagina);
int buscarIndicePaginaCACHE(CACHE *cache, int pid, int nro_pagina);
void actualizarCACHE(cpu_t *cpu, int pid, int nro_pagina, void *contenido);
int hayEntradaVaciaCACHE(CACHE *cache);
void insertarPaginaCACHE(CACHE *cache, int pid, int indice_victima, int nro_pagina, void *contenido);
int seleccionarEntradaVictimaCACHE(CACHE *cache);
void setBitUso(int *bit_uso);
void clearBitUso(int *bit_uso);
void setBitModificado(int *bit_modificado);
void clearBitModificado(int *bit_modificado);
void notificarActualizacionPaginaAMemoria(cpu_t *cpu, int indice_victima);
void *pedirPaginaAMemoria(int socket_memoria, int pid, int direccion_fisica);
void limpiarEntradaCACHE(CACHE *cache, int indice_victima);
void limpiarProcesoCACHE(cpu_t *cpu, int pid);
void marcarModificadoEnCache(CACHE *cache, int pid, int nro_pagina);
void simularRetardoCache();

void inicializarTLB(TLB *tlb);
int buscarPaginaTLB(TLB *tlb, int pid, int nro_pagina);
int buscarIndicePaginaTLB(TLB *tlb, int pid, int nro_pagina);
void actualizarTLB(TLB *tlb, int pid, int nro_pagina, int marco);
void insertarPaginaTLB(TLB *tlb, int pid, int indice_victima, int nro_pagina, int marco);
bool hayEntradaVaciaTLB(TLB *tlb, int *indice_victima);
int traducirDireccionTLB(TLB *tlb, int pid, int direccion_logica);
int seleccionarEntradaVictimaTLB(TLB *tlb);
void limpiarEntradaTLB(TLB *tlb, int indice_victima);
void limpiarProcesoTLB(TLB *tlb, int pid);

enum TIPO_ALGORITMO_REEMPLAZO algoritmo_string_to_enum(char *nombreAlgoritmo);

int getNumeroPagina(int direccion_logica);
int getEntradaNivelX(int nro_pagina, int nro_nivel);
int getDesplazamiento(int direccion_logica);

#endif