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

int buscarMarcoAMemoria(int socket_memoria, int pid, int nro_pagina) {
    int direccion_tabla_actual = -1;

    for(int nivel = 1; nivel <= cantidad_niveles_tabla_paginas; nivel++) {
        int entrada = getEntradaNivelX(nro_pagina, nivel);
            // Che, acuerdense que quedamos en mandar todo de una aca (como un vector de ints de entrada)

        if(nivel == 1) {
            // Es el primer nivel //
            t_paquete *paquete_peticion = crear_paquete(PETICION_LEER_DE_MEMORIA); //
            agregar_a_paquete(paquete_peticion, &pid, sizeof(int));
            agregar_a_paquete(paquete_peticion, &entrada, sizeof(int));
            enviar_paquete(paquete_peticion, socket_memoria);
            eliminar_paquete(paquete_peticion);
            
            direccion_tabla_actual = recibirDireccionTabla(socket_memoria);
        }
        else if(nivel < cantidad_niveles_tabla_paginas) {
            // Es un nivel intermedio //
            t_paquete *paquete_peticion = crear_paquete(PETICION_LEER_DE_MEMORIA); //
            agregar_a_paquete(paquete_peticion, &direccion_tabla_actual, sizeof(int));
            agregar_a_paquete(paquete_peticion, &entrada, sizeof(int));
            enviar_paquete(paquete_peticion, socket_memoria);
            eliminar_paquete(paquete_peticion);

            direccion_tabla_actual = recibirDireccionTabla(socket_memoria);
        }
        else {
            // Es el último nivel //
            t_paquete *paquete_peticion = crear_paquete(PETICION_LEER_DE_MEMORIA); //
            agregar_a_paquete(paquete_peticion, &direccion_tabla_actual, sizeof(int));
            agregar_a_paquete(paquete_peticion, &entrada, sizeof(int));
            enviar_paquete(paquete_peticion, socket_memoria);
            eliminar_paquete(paquete_peticion);

            return recibirMarco(socket_memoria);
        }
    }

    return -1;
}

int recibirDireccionTabla(int socket_memoria) {
    int *codigo_operacion = malloc(sizeof(int));
    t_list* respuesta = recibir_paquete_lista(socket_memoria, MSG_WAITALL, codigo_operacion);
    int direccion_tabla = *((int *)list_get(respuesta, 1));

    free(codigo_operacion);
    eliminar_paquete_lista(respuesta);
    return direccion_tabla;
}

int recibirMarco(int socket_memoria) {
    int *codigo_operacion = malloc(sizeof(int));
    t_list* respuesta = recibir_paquete_lista(socket_memoria, MSG_WAITALL, codigo_operacion);
    int marco = *((int *)list_get(respuesta, 1));

    free(codigo_operacion);
    eliminar_paquete_lista(respuesta);
    return marco;
}

void leerDatoMemoria(int socket_memoria, int pid, int direccion_fisica, int tamanio) {
    t_paquete *paquete_peticion_read = crear_paquete(PETICION_LEER_DE_MEMORIA);
    agregar_a_paquete(paquete_peticion_read, &pid, sizeof(int));
    agregar_a_paquete(paquete_peticion_read, &direccion_fisica, sizeof(int));
    agregar_a_paquete(paquete_peticion_read, &tamanio, sizeof(int));
    enviar_paquete(paquete_peticion_read, socket_memoria);
    eliminar_paquete(paquete_peticion_read);

    int *codigo_operacion = malloc(sizeof(int));
    t_list *respuesta = recibir_paquete_lista(socket_memoria, MSG_WAITALL, codigo_operacion);
    if(respuesta == NULL || *codigo_operacion != RESPUESTA_PETICION) {
        free(codigo_operacion);
        eliminar_paquete_lista(respuesta);
        // Como deberia manejarlo? Deberia salir y avanzar a la siguiente instruccion?
    }
    
    char *leido = strdup((char *)list_get(respuesta, 0)); // no estoy seguro

    printf("READ: %s\n", leido);
    log_info(logger, "PID: %d - Acción: READ - Dirección Física: %d - Valor: %s", pid, direccion_fisica, leido);

    free(codigo_operacion);
    eliminar_paquete_lista(respuesta);
}

void escribirDatoMemoria(int socket_memoria, int pid, int direccion_fisica, char *datos) {
    t_paquete *paquete_peticion_write = crear_paquete(PETICION_ESCRIBIR_EN_MEMORIA);
    agregar_a_paquete(paquete_peticion_write, &pid, sizeof(int));
    agregar_a_paquete(paquete_peticion_write, &direccion_fisica, sizeof(int));
    agregar_a_paquete(paquete_peticion_write, datos, strlen(datos) + 1);
    enviar_paquete(paquete_peticion_write, socket_memoria);
    eliminar_paquete(paquete_peticion_write);

    log_info(logger, "PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %s", pid, direccion_fisica, datos);
}



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

void *buscarPaginaCACHE(CACHE *cache, int pid, int nro_pagina) {
    int pos_pagina = buscarIndicePaginaCACHE(cache, pid, nro_pagina);

    if(pos_pagina == -1) {
        log_info(logger, "PID: %d - Cache Miss - Pagina: %d", pid, nro_pagina);
        return NULL; // CACHE MISS
    }
    
    log_info(logger, "PID: %d - Cache Hit - Pagina: %d", pid, nro_pagina);
    return cache->entradas[pos_pagina].contenido; // CACHE HIT
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

int inicializarTLB(TLB *tlb) {
    if(TLB_SIZE == 0) {
        tlb->habilitada = 0;
        log_debug(logger, "TLB Deshabilitada");
        free(tlb);
        return 0;
    }

    tlb->entradas = (r_TLB *)malloc(sizeof(TLB) + TLB_SIZE * sizeof(r_TLB));

    tlb->habilitada = 1;

    for(int i = 0; i < TLB_SIZE; i++) {
        tlb->entradas[i].pid = -1;
        tlb->entradas[i].pagina = -1;
        tlb->entradas[i].marco = -1;
        tlb->entradas[i].ultimo_uso = -1;
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
    
    tlb->contador_uso++;
    tlb->entradas[pos_registro].ultimo_uso = tlb->contador_uso;

    return tlb->entradas[pos_registro].marco; // TLB HIT
}

int buscarIndicePaginaTLB(TLB *tlb, int pid, int nro_pagina) {
    for(int i = 0; i < TLB_SIZE; i++) {
        if(tlb->entradas[i].validez && tlb->entradas[i].pid == pid && tlb->entradas[i].pagina == nro_pagina)
            return i;
    }

    return -1;
}

void actualizarTLB(TLB *tlb, int pid, int nro_pagina, int marco) {
    // Sucede cuando buscarPaginaTLB() es -1 (TLB MISS) //
    int indice_victima;

    if(hayEntradaVaciaTLB(tlb, &indice_victima)) {
        insertarPaginaTLB(tlb, pid, indice_victima, nro_pagina, marco);
    }
    else {
        indice_victima = seleccionarEntradaVictima(tlb);
        limpiarEntradaTLB(tlb, indice_victima);
        insertarPaginaTLB(tlb, pid, indice_victima, nro_pagina, marco);
    }
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

void insertarPaginaTLB(TLB *tlb, int pid, int indice_victima, int nro_pagina, int marco) {
    tlb->entradas[indice_victima].pid = pid;
    tlb->entradas[indice_victima].pagina = nro_pagina;
    tlb->entradas[indice_victima].marco = marco;

    tlb->contador_uso++;
    tlb->entradas[indice_victima].ultimo_uso = tlb->contador_uso;

    tlb->entradas[indice_victima].validez = 1;
    log_info(logger, "PID: %d - TLB Add - Pagina: %d - Marco: %d", pid, nro_pagina, marco);
}

int seleccionarEntradaVictima(TLB *tlb) {
    int indice_victima = -1;

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
            for(int i = 0; i < TLB_SIZE; i++) {
                int menor_uso;

                if(tlb->entradas[i].validez == 1 && tlb->entradas[i].ultimo_uso < menor_uso) {
                    menor_uso = tlb->entradas[i].ultimo_uso;
                    indice_victima = i;
                }
            }
            
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
    tlb->entradas[indice_victima].pid = -1;
    tlb->entradas[indice_victima].pagina = -1;
    tlb->entradas[indice_victima].marco = -1;
    tlb->entradas[indice_victima].ultimo_uso = -1;
    tlb->entradas[indice_victima].validez = 0;
}

void limpiarProcesoTLB(TLB *tlb, int pid) {
    // Sucede por proceso //
    for(int i = 0; i < TLB_SIZE; i++) {
        if(tlb->entradas[i].validez == 1 && tlb->entradas[i].pid == pid)
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