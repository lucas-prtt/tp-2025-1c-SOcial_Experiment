#ifndef TRADUCCIONES_H
#define TRADUCCIONES_H

#include <commons/collections/list.h>
#include <utils/threads.h>
#include <string.h>
#include "utils/logConfig.h"
#include <math.h>


extern int TLB_SIZE;

extern int cantidad_niveles_tabla_paginas;
extern int cantidad_entradas_tabla;
extern int tamanio_pagina;



typedef struct {
    int pagina;
    int marco;
    int validez; // Para saber si el regitro de la tlb está vacia
    int ultimo_uso;
} r_TLB;

typedef struct {
    int habilitada;
    r_TLB entradas[3]; //hardcodeado
    int proximo; // FIFO
    int contador_uso; // LRU
    int algoritmo;
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
int inicializarTLB(TLB *tlb);
int buscarPaginaTLB(TLB *tlb, int pid, int nro_pagina);
int buscarIndicePaginaTLB(TLB *tlb, int nro_pagina);
void reemplazarEnTLB(TLB *tlb, int nro_pagina, int marco);
void insertarPaginaTLB(TLB *tlb, int indice_victima, int nro_pagina, int marco);
bool hayEntradaVaciaTLB(TLB *tlb, int *indice_victima);
int seleccionarEntradaVictima(TLB *tlb);
void limpiarEntradaTLB(TLB *tlb, int indice_victima);
void vaciarTLB(TLB *tlb);
enum TIPO_ALGORITMO_REEMPLAZO algoritmo_string_to_enum(char *nombreAlgoritmo);
int getNumeroPagina(int direccion_logica);
int getEntradaNivelX(int nro_pagina, int nro_nivel);
int getDesplazamiento(int direccion_logica);

#endif