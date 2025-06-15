#include <commons/collections/list.h>
#include <utils/threads.h>
#include <string.h>
#include "utils/logConfig.h"

//tengo que hacer el tamaño una variable global.
int TLB_SIZE;

int cantidad_niveles_tabla_paginas;
int cantidad_entradas_tabla;
int tamanio_pagina;

static bool variables_globales_inicializadas = false;
static pthread_mutex_t mutex_inicializacion_globales = PTHREAD_MUTEX_INITIALIZER;

////// va a cambiar

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


enum TIPO_ALGORITMO_REEMPLAZO algoritmo_string_to_enum(char *nombreAlgoritmo);