#include "traducciones.h"



/////////////////////////       < VARIABLES GLOBALES >       /////////////////////////

int CACHE_SIZE;
int TLB_SIZE;

int cantidad_niveles_tabla_paginas;
int cantidad_entradas_tabla;
int tamanio_pagina;

void inicializarVariablesGlobales(int socket_memoria, int cant_niveles_t, int cant_entradas_t, int tam_pag) {
    CACHE_SIZE = atoi(config_get_string_value(config, "ENTRADAS_CACHE"));
    TLB_SIZE = atoi(config_get_string_value(config, "ENTRADAS_TLB"));

    tamanio_pagina = tam_pag;
    cantidad_entradas_tabla = cant_entradas_t;
    cantidad_niveles_tabla_paginas = cant_niveles_t;
}



/////////////////////////       < MMU >       /////////////////////////

int traducirDeLogicaAFisica(int direccion_logica) { //
    // Envia a memoria una peticion del tipo: [ PID, Npág, (e1, e2, e3, e4...) ]
}

/*
int leerDatoMemoria(dir_logica dir) {
    return procesar_solicitud(dir,READ_ACCION,0);
}

int escribirDatoMemoria(dir_logica dir, uint32_t dato) {
    return procesar_solicitud(dir, WRITE_ACCION, dato);
}
*/

//sucede despues de no encontrar ni en la tlb ni en cache

/*
// funcion que calcule la entrada de la tabla de paginas en el nivel x (Esta hecha)
//TODO: funcion que envie al modulo memoria el pid de un proceso y un numero de entrada de pagina
        deberia recibir, la base (direccion) del siguiente nivel de la entrada de pagina.
        Comienza otra vez el ciclo, pero con la siguiente entrada a la tabla de paginas y la direccion antes calculada
        hasta obtener el marco de la pagina (puedo usar un for)
*/



/////////////////////////       < CACHÉ DE PÁGINAS >       /////////////////////////

int inicializarCACHE(CACHE *cache) {
    if(CACHE_SIZE > 0) {
        cache->habilitada = 0;
        log_debug(logger, "CACHÉ Deshabilitada");
        free(cache);
        return 0;
    }

    cache->entradas = (r_CACHE *)malloc(sizeof(CACHE) + CACHE_SIZE * sizeof(r_CACHE));

    cache->habilitada = 1;

    cache->algoritmo = algoritmo_string_to_enum(config_get_string_value(config, "REEMPLAZO_CACHE"));
    
    log_debug(logger, "CACHÉ Habilitada");
    return 1;
}

int buscarPaginaCACHE(CACHE *cache, int pid, int nro_pagina) {
    int pos_pagina = buscarIndicePaginaCACHE(cache, pid, nro_pagina);

    if(pos_pagina == -1) {
        log_info(logger, "PID: %d - Cache Miss - Pagina: %d", pid, nro_pagina);
        return -1; // CACHE MISS
    }
    
    log_info(logger, "PID: %d - Cache Hit - Pagina: %d", pid, nro_pagina);
    return pos_pagina; // CACHE HIT
}

int buscarIndicePaginaCACHE(CACHE *cache, int pid, int nro_pagina) {
    for(int i = 0; i < CACHE_SIZE; i++) {
        if(cache->entradas[i].contenido && cache->entradas[i].pid == pid && cache->entradas[i].pagina == nro_pagina) {
            cache->entradas[i].bit_uso = 1; // de que me sirve?
            return i;
        }
    }

    return -1;
}










/////////////////////////       < TLB >       /////////////////////////

int inicializarTLB(TLB *tlb) { // crearTLB(TLB *tlb)
    if(TLB_SIZE == 0) {
        tlb->habilitada = 0;
        log_debug(logger, "TLB Deshabilitada");
        free(tlb);
        return 0;
    }

    tlb->entradas = (r_TLB *)malloc(sizeof(TLB) + TLB_SIZE * sizeof(r_TLB));

    tlb->habilitada = 1;

    for(int i = 0; i < TLB_SIZE; i++) {
        tlb->entradas[i].validez = 0;
    }

    tlb->proximo = 0;
    tlb->algoritmo = algoritmo_string_to_enum(config_get_string_value(config, "REEMPLAZO_TLB"));
    
    log_debug(logger, "TLB Habilitada");
    return 1;
}

int buscarPaginaTLB(TLB *tlb, int pid, int nro_pagina) {
    int pos_registro = buscarIndicePaginaTLB(tlb, pid, nro_pagina);

    if(pos_registro == -1) {
        log_info(logger, "PID: %d - TLB MISS - Pagina: %d", pid, nro_pagina);
        return -1; // TLB MISS
    }
    
    log_info(logger, "PID: %d - TLB HIT - Pagina: %d", pid, nro_pagina);
    return tlb->entradas[pos_registro].marco; // TLB HIT
}

int buscarIndicePaginaTLB(TLB *tlb, int pid, int nro_pagina) { // cambia porque pueden habe varios pid
    for(int i = 0; i < TLB_SIZE; i++) {
        if(tlb->entradas[i].validez && tlb->entradas[i].pid == pid && tlb->entradas[i].pagina == nro_pagina)
            return i;
    }

    return -1;
}

void reemplazarEnTLB(TLB *tlb, int pid, int nro_pagina, int marco) {
    // Sucede cuando buscarPaginaTLB() es -1 (TLB MISS) //
    int indice_victima;

    if(hayEntradaVaciaTLB(tlb, &indice_victima)) {
        insertarPaginaTLB(tlb, pid, indice_victima, nro_pagina, marco);
    }
    
    indice_victima = seleccionarEntradaVictima(tlb);
    limpiarEntradaTLB(tlb, indice_victima);
    insertarPaginaTLB(tlb, pid, indice_victima, nro_pagina, marco);
}

void insertarPaginaTLB(TLB *tlb, int pid, int indice_victima, int nro_pagina, int marco) {
    tlb->entradas[indice_victima].pid = pid;
    tlb->entradas[indice_victima].pagina = nro_pagina;
    tlb->entradas[indice_victima].marco = marco;
    tlb->entradas[indice_victima].ultimo_uso = -1;
    tlb->entradas[indice_victima].validez = 1;
    log_info(logger, "PID: %d - TLB Add - Pagina: %d - Marco: %d", pid, nro_pagina, marco);
}

bool hayEntradaVaciaTLB(TLB *tlb, int *indice_victima) {
    for(int i = 0; i < TLB_SIZE; i++) {
        if(tlb->entradas[i].validez == 0) {
            *indice_victima = i;
            return true;
        }
    }
    return false;
}

int seleccionarEntradaVictima(TLB *tlb) {
    int indice_victima = 0;

    switch(tlb->algoritmo)
    {
        case ALG_FIFO:
        {
            indice_victima = tlb->proximo;
            tlb->proximo = (tlb->proximo + 1) % TLB_SIZE;

            return indice_victima;
        }
        case ALG_LRU:
        {
            int menor_uso = tlb->entradas[0].ultimo_uso;

            for(int i = 1; i < TLB_SIZE; i++) {
                if(tlb->entradas[i].ultimo_uso < menor_uso) {
                    menor_uso = tlb->entradas[i].ultimo_uso;
                    indice_victima = i;
                }
            }
            // contador_uso: cada que tlb_hit o reemplazo
            return indice_victima;
        }
        default:
        {
            log_error(logger, "Algoritmo de reemplazo desconocido");
            return 0;
        }
    }
}

void limpiarEntradaTLB(TLB *tlb, int indice_victima) {
    tlb->entradas[indice_victima].pagina = -1;
    tlb->entradas[indice_victima].marco = -1;
    tlb->entradas[indice_victima].ultimo_uso = -1;
    tlb->entradas[indice_victima].validez = 0;
}

void limpiarProcesoTLB(TLB *tlb, int pid) {
    // Sucede por proceso //
    for(int i = 0; i < TLB_SIZE; i++) {
        if(tlb->entradas[i].pid == pid)
            limpiarEntradaTLB(tlb, i);
    }
}



/////////////////////////       < UTILS >       /////////////////////////

enum TIPO_ALGORITMO_REEMPLAZO algoritmo_string_to_enum(char *nombreAlgoritmo) {
    if (!strcmp(nombreAlgoritmo, "FIFO")) {
        return ALG_FIFO;
    }
    if (!strcmp(nombreAlgoritmo, "LRU")) {
        return ALG_LRU;
    }
    if (!strcmp(nombreAlgoritmo, "CLOCK")) {
        return ALG_CLOCK;
    }
    if (!strcmp(nombreAlgoritmo, "CLOCK-M")) {
        return ALG_CLOCK_M;
    }
    return ERROR_NO_ALG;
}



/////////////////////////       < TRADUCCIÓN >       /////////////////////////

int getNumeroPagina(int direccion_logica) {
    return floor(direccion_logica / tamanio_pagina);
}

int getEntradaNivelX(int nro_pagina, int nro_nivel) {
    int resultado = (int)pow(cantidad_entradas_tabla, cantidad_niveles_tabla_paginas - nro_nivel);
    return (int)floor(nro_pagina / resultado) % cantidad_entradas_tabla;
}

int getDesplazamiento(int direccion_logica) {
    return direccion_logica % tamanio_pagina;
}