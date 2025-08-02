#include "traducciones.h"
#include "utils_cpu.h"


/////////////////////////       < VARIABLES GLOBALES >       /////////////////////////

int CACHE_SIZE;
int CACHE_RETARDO;
int TLB_SIZE;

int cantidad_niveles_tabla_paginas;
int cantidad_entradas_tabla;
int tamanio_pagina;

void inicializarVariablesGlobales(int socket_memoria, int cant_niveles_t, int cant_entradas_t, int tam_pag) {
    CACHE_SIZE = config_get_int_value(config, "ENTRADAS_CACHE");
    CACHE_RETARDO = config_get_int_value(config, "RETARDO_CACHE");
    TLB_SIZE = config_get_int_value(config, "ENTRADAS_TLB");

    tamanio_pagina = tam_pag;
    cantidad_entradas_tabla = cant_entradas_t;
    cantidad_niveles_tabla_paginas = cant_niveles_t;
}



/////////////////////////       < MMU >       /////////////////////////

int traducirDireccionMMU(cpu_t *cpu, int pid, int direccion_logica) {
    int marco = -1;
    int direccion_fisica = -1;
    int nro_pagina = getNumeroPagina(direccion_logica);
    if(cpu->tlb->habilitada) {
        direccion_fisica = traducirDireccionTLB(cpu->tlb, pid, direccion_logica);
        if(direccion_fisica == -1) {
            marco = buscarMarcoAMemoria(cpu->socket_memoria, pid, nro_pagina);
            actualizarTLB(cpu->tlb, pid, nro_pagina, marco);
            direccion_fisica = traducirDireccionTLB(cpu->tlb, pid, direccion_logica);
        }
    }
    else {
        marco = buscarMarcoAMemoria(cpu->socket_memoria, pid, nro_pagina);
        direccion_fisica = traducirDireccion(marco);
    }

    //log_warning(logger, "Traduzco DL %d a DF %d", direccion_logica, direccion_fisica);
    return direccion_fisica;
}

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

int traducirDireccion(int marco) {
    return marco * tamanio_pagina;
}

int buscarMarcoAMemoria(int socket_memoria, int pid, int nro_pagina) {
    int entradas[cantidad_niveles_tabla_paginas];

    for(int nivel = 1; nivel <= cantidad_niveles_tabla_paginas; nivel++) {
        entradas[nivel - 1] = getEntradaNivelX(nro_pagina, nivel);
    }

    t_paquete *paquete_peticion_marco = crear_paquete(PETICION_MARCO_MEMORIA);
    agregar_a_paquete(paquete_peticion_marco, &pid, sizeof(int));
    agregar_a_paquete(paquete_peticion_marco, entradas, sizeof(int) * cantidad_niveles_tabla_paginas);
    enviar_paquete(paquete_peticion_marco, socket_memoria);
    eliminar_paquete(paquete_peticion_marco);

    int *codigo_operacion = malloc(sizeof(int));
    t_list* respuesta = recibir_paquete_lista(socket_memoria, MSG_WAITALL, codigo_operacion);
    if(*codigo_operacion == RESPUESTA_MEMORIA_A_CPU_PAGINA_NO_VALIDA){
        log_error(logger, "Error catastrofico. Se quiso leer o escribir una pagina fuera de rango");
        abort();
    }
    int marco = *((int *)list_get(respuesta, 1));

    free(codigo_operacion);
    eliminar_paquete_lista(respuesta);
    return marco;
}

void escribirEnMemoria(cpu_t *cpu, int pid, int direccion_logica, char *datos) {
    int bytes_restantes = strlen(datos);
    char *puntero_datos = datos;

    while(bytes_restantes > 0) {
        // int nro_pagina_actual = getNumeroPagina(direccion_logica);
        int desplazamiento = getDesplazamiento(direccion_logica);
        int espacio_pagina_actual = tamanio_pagina - desplazamiento;
        int bytes_a_escribir;

        if(bytes_restantes < espacio_pagina_actual) {
            bytes_a_escribir = bytes_restantes;
        }
        else {
            bytes_a_escribir = espacio_pagina_actual;
        }

        int direccion_fisica = traducirDireccionMMU(cpu, pid, direccion_logica);

        escribirSeccionPaginaEnMemoria(cpu->socket_memoria, pid, direccion_fisica + desplazamiento, puntero_datos, bytes_a_escribir);

        puntero_datos += bytes_a_escribir;
        bytes_restantes -= bytes_a_escribir;
        direccion_logica += bytes_a_escribir;
    }
}

void escribirPaginaCompletaEnMemoria(int socket_memoria, int pid, int direccion_fisica, char *datos) {
    t_paquete *paquete_peticion_write = crear_paquete(PETICION_ESCRIBIR_EN_MEMORIA);
    agregar_a_paquete(paquete_peticion_write, &pid, sizeof(int));
    agregar_a_paquete(paquete_peticion_write, &direccion_fisica, sizeof(int));
    agregar_a_paquete(paquete_peticion_write, datos, tamanio_pagina);
    enviar_paquete(paquete_peticion_write, socket_memoria);
    eliminar_paquete(paquete_peticion_write);

    int *codigo_operacion = malloc(sizeof(int));
    t_list* respuesta = recibir_paquete_lista(socket_memoria, MSG_WAITALL, codigo_operacion);
    if(*codigo_operacion == RESPUESTA_ESCRIBIR_EN_MEMORIA) {
        log_info(logger, "PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %s", pid, direccion_fisica, datos);
    }

    free(codigo_operacion);
    list_destroy_and_destroy_elements(respuesta, free);
}

void escribirSeccionPaginaEnMemoria(int socket_memoria, int pid, int direccion_fisica, char *datos, int tamanio_datos) {
    t_paquete *paquete_peticion_write = crear_paquete(PETICION_ESCRIBIR_EN_MEMORIA_LIMITADO);
    agregar_a_paquete(paquete_peticion_write, &pid, sizeof(int));
    agregar_a_paquete(paquete_peticion_write, &direccion_fisica, sizeof(int));
    agregar_a_paquete(paquete_peticion_write, datos, tamanio_datos);
    agregar_a_paquete(paquete_peticion_write, &tamanio_datos, sizeof(int));
    enviar_paquete(paquete_peticion_write, socket_memoria);
    eliminar_paquete(paquete_peticion_write);

    int *codigo_operacion = malloc(sizeof(int));
    t_list* respuesta = recibir_paquete_lista(socket_memoria, MSG_WAITALL, codigo_operacion);
    if(*codigo_operacion == RESPUESTA_ESCRIBIR_EN_MEMORIA) {
        log_info(logger, "PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %s", pid, direccion_fisica, datos);
    }

    free(codigo_operacion);
    eliminar_paquete_lista(respuesta);
}

void leerDeMemoria(cpu_t *cpu, int pid, int direccion_logica, int tamanio) {
    int bytes_restantes = tamanio;
    int datos_leidos = 0;

    while(bytes_restantes > 0) {
        // int nro_pagina_actual = getNumeroPagina(direccion_logica);
        int desplazamiento = getDesplazamiento(direccion_logica);
        int espacio_pagina_actual = tamanio_pagina - desplazamiento;
        int bytes_a_leer;

        if(bytes_restantes < espacio_pagina_actual) {
            bytes_a_leer = bytes_restantes;
        }
        else {
            bytes_a_leer = espacio_pagina_actual;
        }

        int direccion_fisica = traducirDireccionMMU(cpu, pid, direccion_logica);

        leerSeccionPaginaMemoria(cpu->socket_memoria, pid, direccion_fisica + desplazamiento, bytes_a_leer);

        datos_leidos += bytes_a_leer;
        bytes_restantes -= bytes_a_leer;
        direccion_logica += bytes_a_leer;
    }
}

// No se esta utilizando... //
void leerPaginaCompletaMemoria(int socket_memoria, int pid, int direccion_fisica, int tamanio) {
    t_paquete *paquete_peticion_read = crear_paquete(PETICION_LEER_DE_MEMORIA);
    agregar_a_paquete(paquete_peticion_read, &pid, sizeof(int));
    agregar_a_paquete(paquete_peticion_read, &direccion_fisica, sizeof(int));
    agregar_a_paquete(paquete_peticion_read, &tamanio, sizeof(int));
    enviar_paquete(paquete_peticion_read, socket_memoria);
    eliminar_paquete(paquete_peticion_read);

    int *codigo_operacion = malloc(sizeof(int));
    t_list *respuesta = recibir_paquete_lista(socket_memoria, MSG_WAITALL, codigo_operacion);
    if(respuesta == NULL || *codigo_operacion != RESPUESTA_LEER_DE_MEMORIA) {
        free(codigo_operacion);
        eliminar_paquete_lista(respuesta);
    }
    
    char *leido = strndup((char *)list_get(respuesta, 1), tamanio);

    //printf("READ: %s\n", leido);
    //log_info(logger, "PID: %d - Acción: READ - Dirección Física: %d - Valor: %s", pid, direccion_fisica, leido);

    free(leido);
    free(codigo_operacion);
    eliminar_paquete_lista(respuesta);
}

void leerSeccionPaginaMemoria(int socket_memoria, int pid, int direccion_fisica, int tamanio) {
    t_paquete *paquete_peticion_read = crear_paquete(PETICION_LEER_DE_MEMORIA_LIMITADO);
    agregar_a_paquete(paquete_peticion_read, &pid, sizeof(int));
    agregar_a_paquete(paquete_peticion_read, &direccion_fisica, sizeof(int));
    agregar_a_paquete(paquete_peticion_read, &tamanio, sizeof(int));
    enviar_paquete(paquete_peticion_read, socket_memoria);
    eliminar_paquete(paquete_peticion_read);

    int *codigo_operacion = malloc(sizeof(int));
    t_list *respuesta = recibir_paquete_lista(socket_memoria, MSG_WAITALL, codigo_operacion);
    if(respuesta == NULL || *codigo_operacion != RESPUESTA_LEER_DE_MEMORIA) {
        free(codigo_operacion);
        eliminar_paquete_lista(respuesta);
    }
    
    char *leido = strndup((char *)list_get(respuesta, 1), tamanio);

    printf("READ: %s\n", leido);
    log_info(logger, "PID: %d - Acción: LEER - Dirección Física: %d - Valor: %s", pid, direccion_fisica, leido);

    free(leido);
    free(codigo_operacion);
    eliminar_paquete_lista(respuesta);
}



/////////////////////////       < CACHÉ DE PÁGINAS >       /////////////////////////

void inicializarCACHE(CACHE *cache) {
    if(CACHE_SIZE < 1) {
        cache->habilitada = 0;
        cache->entradas = NULL;
        log_debug(logger, "Cache Deshabilitada");
        return;
    }
    
    cache->habilitada = 1;
    cache->puntero_clock = 0;
    cache->algoritmo = algoritmo_string_to_enum(config_get_string_value(config, "REEMPLAZO_CACHE"));
    
    cache->entradas = malloc(CACHE_SIZE * sizeof(r_CACHE));
    for(int i = 0; i < CACHE_SIZE; i++) {
        cache->entradas[i].contenido = NULL;
        cache->entradas[i].pid = -1;
        cache->entradas[i].pagina = -1;
        cache->entradas[i].bit_modificado = 0;
        cache->entradas[i].bit_uso = 0;
    }

    log_debug(logger, "Cache Habilitada");
}

void *buscarPaginaCACHE(CACHE *cache, int pid, int nro_pagina) {
    simularRetardoCache();

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
            cache->entradas[i].bit_uso = 1;
            return i;
        }
    }

    return -1;
}

void actualizarCACHE(cpu_t *cpu, int pid, int nro_pagina, void *contenido) {
    // Se ejecuta sabiendo que la página no está en la caché //
    simularRetardoCache();

    // CASO 1: hay registros vacios //
    int indice_victima = hayEntradaVaciaCACHE(cpu->cache);
    if(indice_victima != -1) {
        insertarPaginaCACHE(cpu->cache, pid, indice_victima, nro_pagina, contenido);
        cpu->cache->puntero_clock = (cpu->cache->puntero_clock + 1) % CACHE_SIZE;
    }
    // CASO 2: no hay registros vacios //
    else {
        indice_victima = seleccionarEntradaVictimaCACHE(cpu->cache);
        if(cpu->cache->entradas[indice_victima].bit_modificado) {
            notificarActualizacionPaginaAMemoria(cpu, indice_victima);
        }
        limpiarEntradaCACHE(cpu->cache, indice_victima);
        insertarPaginaCACHE(cpu->cache, pid, indice_victima, nro_pagina, contenido);
    }
}

int hayEntradaVaciaCACHE(CACHE *cache) {
    for(int i = 0; i < CACHE_SIZE; i++) {
        if(cache->entradas[i].contenido == NULL) {
            return i;
        }
    }

    return -1;
}

void insertarPaginaCACHE(CACHE *cache, int pid, int indice_victima, int nro_pagina, void *contenido) {
    cache->entradas[indice_victima].pid = pid;
    cache->entradas[indice_victima].pagina = nro_pagina;
    if(cache->entradas[indice_victima].contenido != NULL) {
        free(cache->entradas[indice_victima].contenido);
    }
    cache->entradas[indice_victima].contenido = malloc(tamanio_pagina);

    memcpy(cache->entradas[indice_victima].contenido, contenido, tamanio_pagina);
    // ((char*)cache->entradas[indice_victima].contenido)[tamanio_pagina] = '\0';
    
    cache->entradas[indice_victima].bit_uso = 1;

    log_info(logger, "PID: %d - Cache Add - Pagina: %d", cache->entradas[indice_victima].pid, cache->entradas[indice_victima].pagina);
}

int seleccionarEntradaVictimaCACHE(CACHE *cache) {
    switch(cache->algoritmo)
    {
        case ALG_CLOCK:
        {
            int indice_victima = -1;
            
            while(true) {
                r_CACHE *registro_actual = &cache->entradas[cache->puntero_clock];

                if(registro_actual->bit_uso == 0) {
                    // Encontramos a la victima //
                    indice_victima = cache->puntero_clock;
                    cache->puntero_clock = (cache->puntero_clock + 1) % CACHE_SIZE;
                    return indice_victima;
                }
                else {
                    registro_actual->bit_uso = 0;
                    cache->puntero_clock = (cache->puntero_clock + 1) % CACHE_SIZE;
                }
            }
        }
        case ALG_CLOCK_M:
        {
            int vueltas = 0;

            while(vueltas < 2) {
                // Busca: (0, 0) //
                for(int i = 0; i < CACHE_SIZE; i++) {
                    int indice = (cache->puntero_clock + i) % CACHE_SIZE;
                    r_CACHE *entrada = &cache->entradas[indice];

                    if(entrada->bit_uso == 0 && entrada->bit_modificado == 0) {
                        cache->puntero_clock = (indice + 1) % CACHE_SIZE;
                        return indice;
                    }
                }

                // Busca: (0, 1) //
                for(int i = 0; i < CACHE_SIZE; i++) {
                    int indice = (cache->puntero_clock + i) % CACHE_SIZE;
                    r_CACHE *entrada = &cache->entradas[indice];

                    if(entrada->bit_uso == 0 && entrada->bit_modificado == 1) {
                        cache->puntero_clock = (indice + 1) % CACHE_SIZE;
                        return indice;
                    }

                    cache->entradas[i].bit_uso = 0;
                }

                /* Si no se encontró víctima, limpiar bits de uso y repetir //
                for(int i = 0; i < CACHE_SIZE; i++) {
                    cache->entradas[i].bit_uso = 0;
                }*/

                vueltas++;
            }

            log_error(logger, "CLOCK-M: No se encontró víctima luego de 2 vueltas");
            return -1;
        }
        default:
        {
            log_error(logger, "Algoritmo de reemplazo desconocido");
            return 0;
        }
    }

    return -1;
}

void setBitUso(int *bit_uso) {
    *bit_uso = 1;
}

void clearBitUso(int *bit_uso) {
    *bit_uso = 0;
}

void setBitModificado(int *bit_modificado) {
    *bit_modificado = 1;
}

void clearBitModificado(int *bit_modificado) {
    *bit_modificado = 0;
}

void notificarActualizacionPaginaAMemoria(cpu_t *cpu, int indice_victima) {
    r_CACHE *entrada = &cpu->cache->entradas[indice_victima];

    int direccion_fisica = traducirDireccionMMU(cpu, entrada->pid, entrada->pagina * tamanio_pagina);
    escribirPaginaCompletaEnMemoria(cpu->socket_memoria, entrada->pid, direccion_fisica, entrada->contenido);
    log_info(logger, "PID: %d - Memory Update - Página: %d - Frame: %d", entrada->pid, entrada->pagina, (direccion_fisica / tamanio_pagina));
}

void *pedirPaginaAMemoria(int socket_memoria, int pid, int direccion_fisica) {
    t_paquete *paquete_peticion_pagina = crear_paquete(PETICION_LEER_DE_MEMORIA);
    agregar_a_paquete(paquete_peticion_pagina, &pid, sizeof(int));
    agregar_a_paquete(paquete_peticion_pagina, &direccion_fisica, sizeof(int));
    agregar_a_paquete(paquete_peticion_pagina, &tamanio_pagina, sizeof(int));
    enviar_paquete(paquete_peticion_pagina, socket_memoria);
    eliminar_paquete(paquete_peticion_pagina);

    int *codigo_operacion = malloc(sizeof(int));
    t_list *respuesta = recibir_paquete_lista(socket_memoria, MSG_WAITALL, codigo_operacion);
    if (!respuesta || *codigo_operacion != RESPUESTA_LEER_DE_MEMORIA || list_size(respuesta) < 1) {
        eliminar_paquete_lista(respuesta);
        free(codigo_operacion);
        return NULL;
    }
    
    void *pagina = malloc(tamanio_pagina);
    memcpy(pagina, list_get(respuesta, 1), tamanio_pagina);

    eliminar_paquete_lista(respuesta);
    free(codigo_operacion);
    return pagina;
}

void limpiarEntradaCACHE(CACHE *cache, int indice_victima) {
    cache->entradas[indice_victima].pid = -1;
    cache->entradas[indice_victima].pagina = -1;
    if(cache->entradas[indice_victima].contenido != NULL) {
        free(cache->entradas[indice_victima].contenido);
        cache->entradas[indice_victima].contenido = NULL;
    }
    cache->entradas[indice_victima].bit_uso = 0;
    cache->entradas[indice_victima].bit_modificado = 0;
}

void limpiarProcesoCACHE(cpu_t *cpu, int pid) {
    for(int i = 0; i < CACHE_SIZE; i++) {
        if(cpu->cache->entradas[i].pid == pid) {
            if(cpu->cache->entradas[i].bit_modificado) {
                notificarActualizacionPaginaAMemoria(cpu, i);
            }
            limpiarEntradaCACHE(cpu->cache, i);
        }
    }
}

void escribirEnCache(cpu_t *cpu, int pid, int direccion_logica, char *datos) {
    int bytes_restantes = strlen(datos);
    char *puntero_datos = datos;

    while(bytes_restantes > 0) {
        int nro_pagina_actual = getNumeroPagina(direccion_logica);
        int desplazamiento = getDesplazamiento(direccion_logica);
        int espacio_pagina_actual = tamanio_pagina - desplazamiento;
        int bytes_a_escribir;

        if(bytes_restantes < espacio_pagina_actual) {
            bytes_a_escribir = bytes_restantes;
        }
        else {
            bytes_a_escribir = espacio_pagina_actual;
        }

        void *contenido_cache = buscarPaginaCACHE(cpu->cache, pid, nro_pagina_actual);
        if(contenido_cache == NULL) {
            int direccion_fisica = traducirDireccionMMU(cpu, pid, direccion_logica);
            void *pagina = pedirPaginaAMemoria(cpu->socket_memoria, pid, direccion_fisica);

            void *paginaComoCorresponde = malloc(tamanio_pagina);
            memcpy(paginaComoCorresponde, pagina, tamanio_pagina);
            memcpy(paginaComoCorresponde + desplazamiento, datos, bytes_a_escribir);
            actualizarCACHE(cpu, pid, nro_pagina_actual, paginaComoCorresponde);
            contenido_cache = buscarPaginaCACHE(cpu->cache, pid, nro_pagina_actual);
            
            free(pagina);
            free(paginaComoCorresponde);
        }
        
        memcpy((char *)contenido_cache + desplazamiento, puntero_datos, bytes_a_escribir);
        marcarModificadoEnCache(cpu->cache, pid, nro_pagina_actual);

        puntero_datos += bytes_a_escribir;
        bytes_restantes -= bytes_a_escribir;
        direccion_logica += bytes_a_escribir;
    }
}

void leerDeCache(cpu_t *cpu, int pid, int direccion_logica, int tamanio) {
    int bytes_restantes = tamanio;
    int datos_leidos = 0;
    char * leido = malloc(tamanio_pagina+1);
    leido[tamanio_pagina] = '\0';
    
    while(bytes_restantes > 0) {
        int nro_pagina_actual = getNumeroPagina(direccion_logica);
        int desplazamiento = getDesplazamiento(direccion_logica);
        int espacio_pagina_actual = tamanio_pagina - desplazamiento;
        int bytes_a_leer;

        if(bytes_restantes < espacio_pagina_actual) {
            bytes_a_leer = bytes_restantes;
        }
        else {
            bytes_a_leer = espacio_pagina_actual;
        }

        int direccion_fisica = traducirDireccionMMU(cpu, pid, direccion_logica);
            
        void *contenido_cache = buscarPaginaCACHE(cpu->cache, pid, nro_pagina_actual);
        if(contenido_cache == NULL) {
            void *pagina = pedirPaginaAMemoria(cpu->socket_memoria, pid, direccion_fisica);
            actualizarCACHE(cpu, pid, nro_pagina_actual, pagina);
            contenido_cache = buscarPaginaCACHE(cpu->cache, pid, nro_pagina_actual);
            free(pagina);
        }

        memcpy(leido, (char *)contenido_cache + desplazamiento, bytes_a_leer);
        leido[bytes_a_leer] = '\0';
        printf("READ: %s\n", leido);
        log_info(logger, "PID: %d - Acción: LEER - Dirección Física: %d - Valor: %s", pid, direccion_fisica, leido);
        
        datos_leidos += bytes_a_leer;
        bytes_restantes -= bytes_a_leer;
        direccion_logica += bytes_a_leer;
    }
        free(leido);
}

void simularRetardoCache() {
    usleep(CACHE_RETARDO * 1000);
}

/////////////////////////       < TLB >       /////////////////////////

void inicializarTLB(TLB *tlb) {
    if(TLB_SIZE == 0) {
        tlb->habilitada = 0;
        tlb->entradas = NULL;
        log_debug(logger, "TLB Deshabilitada");
        return;
    }

    tlb->habilitada = 1;
    tlb->proximo = 0;
    tlb->contador_uso = 0;

    tlb->entradas = malloc(TLB_SIZE * sizeof(r_TLB));
    for(int i = 0; i < TLB_SIZE; i++) {
        tlb->entradas[i].pid = -1;
        tlb->entradas[i].pagina = -1;
        tlb->entradas[i].marco = -1;
        tlb->entradas[i].ultimo_uso = -1;
    }

    tlb->algoritmo = algoritmo_string_to_enum(config_get_string_value(config, "REEMPLAZO_TLB"));
    
    log_debug(logger, "TLB Habilitada");
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
        if(tlb->entradas[i].pid == pid && tlb->entradas[i].pagina == nro_pagina) {
            return i;
        }
    }

    return -1;
}

void actualizarTLB(TLB *tlb, int pid, int nro_pagina, int marco) {
    // Se ejecuta sabiendo que la página no está en la TLB //
    int indice_victima;

    if(hayEntradaVaciaTLB(tlb, &indice_victima)) {
        insertarPaginaTLB(tlb, pid, indice_victima, nro_pagina, marco);
    }
    else {
        indice_victima = seleccionarEntradaVictimaTLB(tlb);
        limpiarEntradaTLB(tlb, indice_victima);
        insertarPaginaTLB(tlb, pid, indice_victima, nro_pagina, marco);
    }
}

int traducirDireccionTLB(TLB *tlb, int pid, int direccion_logica) {
    int nro_pagina = getNumeroPagina(direccion_logica);
    int marco = -1;

    marco = buscarPaginaTLB(tlb, pid, nro_pagina);

    if(marco != -1) {
        log_info(logger, "PID: %d - OBTENER MARCO - Página: %d - Marco: %d", pid, nro_pagina, marco);
        return traducirDireccion(marco);
    }

    return -1;
}

bool hayEntradaVaciaTLB(TLB *tlb, int *indice_victima) {
    for(int i = 0; i < TLB_SIZE; i++) {
        if(tlb->entradas[i].pagina == -1) {
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

    log_debug(logger, "PID: %d - TLB Add - Pagina: %d", tlb->entradas[indice_victima].pid, tlb->entradas[indice_victima].pagina);
}

int seleccionarEntradaVictimaTLB(TLB *tlb) {
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
            int menor_uso = INT_MAX;

            for(int i = 0; i < TLB_SIZE; i++) {
                if(tlb->entradas[i].ultimo_uso < menor_uso) {
                    menor_uso = tlb->entradas[i].ultimo_uso;
                    indice_victima = i;
                }
            }
            
            return indice_victima;
        }
        default:
        {
            log_error(logger, "Algoritmo de reemplazo desconocido");
            return -1;
        }
    }
}

void limpiarEntradaTLB(TLB *tlb, int indice_victima) {
    tlb->entradas[indice_victima].pid = -1;
    tlb->entradas[indice_victima].pagina = -1;
    tlb->entradas[indice_victima].marco = -1;
    tlb->entradas[indice_victima].ultimo_uso = -1;
}

void limpiarProcesoTLB(TLB *tlb, int pid) {
    // Sucede por proceso //
    for(int i = 0; i < TLB_SIZE; i++) {
        if(tlb->entradas[i].pid == pid) {
            limpiarEntradaTLB(tlb, i);
        }
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