#include "utils_cpu.h"


/////////////////////////       < SOCKETS >       /////////////////////////

int generarSocket(char *ip_cliente, char *puerto_cliente, char *modulo_cliente) {
    int socket = conectarSocketClient(ip_cliente, puerto_cliente);
    verificarConexionCliente(socket, modulo_cliente);
    return socket;
}

void verificarConexionCliente(int socket_cliente, char *nombreModuloCliente) {
    if(socket_cliente == -1) {
        log_error(logger, "Conexión Inicial - %s - Error", nombreModuloCliente);
        exit(EXIT_FAILURE);
    }
    else {
        log_debug(logger, "Conexión Inicial - %s - Success", nombreModuloCliente);
    }
}



/////////////////////////       < HANDSHAKE - MEMORIA >       /////////////////////////

void realizarHandshakeMemoria(int socket_cliente, int identificadorCPU, char* modulo_cliente) {
    bool resultHandshake = handshakeMemoria(socket_cliente, identificadorCPU);
    verificarResultadoHandshake(resultHandshake, modulo_cliente);
}

bool handshakeMemoria(int socket_memoria, int identificador) {
    t_paquete* paquete_saludo_memoria = crear_paquete(SOYCPU);
    enviar_paquete(paquete_saludo_memoria, socket_memoria);
    eliminar_paquete(paquete_saludo_memoria);

    int respuesta;
    t_list *lista_contenido = recibir_paquete_lista(socket_memoria, MSG_WAITALL, &respuesta);
    if(lista_contenido == NULL || respuesta != SOYMEMORIA) {
        eliminar_paquete_lista(lista_contenido);
        return false;
    }
    

    inicializarVariablesGlobales(socket_memoria, 
                                *(int *)list_get(lista_contenido, 1),
                                *(int *)list_get(lista_contenido, 3),
                                *(int *)list_get(lista_contenido, 5)
                                );

    log_debug(logger, "Crear variables globales - Success:");
    log_debug(logger, "  Tamaño caché             : %d", CACHE_SIZE);
    log_debug(logger, "  Tamaño TLB               : %d", TLB_SIZE);
    log_debug(logger, "  Tamaño de página         : %d", tamanio_pagina);
    log_debug(logger, "  Entradas por tabla       : %d", cantidad_entradas_tabla);
    log_debug(logger, "  Cantidad de niveles      : %d", cantidad_niveles_tabla_paginas);

    eliminar_paquete_lista(lista_contenido);
    return true;
}



/////////////////////////       < HANDSHAKE - KERNEL >       /////////////////////////

void realizarHandshakeKernel(int socket_cliente, int identificadorCPU, char* modulo_cliente) {
    bool resultHandshake = handshakeKernel(socket_cliente, identificadorCPU);
    verificarResultadoHandshake(resultHandshake, modulo_cliente);
}

bool handshakeKernel(int socket_kernel, int identificador) {
    t_paquete* paquete_consult_cliente = crear_paquete(HANDSHAKE);
    agregar_a_paquete(paquete_consult_cliente, &identificador, sizeof(int));
    enviar_paquete(paquete_consult_cliente, socket_kernel);
    eliminar_paquete(paquete_consult_cliente);

    int *codigo_operacion = malloc(sizeof(int));
    t_list *lista_contenido = recibir_paquete_lista(socket_kernel, MSG_WAITALL, codigo_operacion);
    if(lista_contenido == NULL || list_size(lista_contenido) < 2 || *codigo_operacion != HANDSHAKE) {
        free(codigo_operacion);
        eliminar_paquete_lista(lista_contenido);
        return false;
    }

    int result = false;
    if(*(int*)list_get(lista_contenido, 1) == identificador) {
        result = true;
    }

    free(codigo_operacion);
    eliminar_paquete_lista(lista_contenido);
    return result;
}



/////////////////////////       < HANDSHAKE - UTILS >       /////////////////////////

void verificarResultadoHandshake(bool result, char* nombreModuloCliente) {
    if(result) {
        log_debug(logger, "Handshake - %s - Success", nombreModuloCliente);
    }
    else {
        log_error(logger, "Handshake - %s - Error", nombreModuloCliente);
        exit(EXIT_FAILURE);
    }
}



/////////////////////////       < OTROS >       /////////////////////////

cpu_t *prepararCPU(int socket_memoria, int socket_kernel_dispatch, int socket_kernel_interrupt) {
    cpu_t *cpu = malloc(sizeof(cpu_t));
    
    cpu->interrupciones = list_create();
    cpu->socket_memoria = socket_memoria;
    cpu->socket_kernel_dispatch = socket_kernel_dispatch;
    cpu->socket_kernel_interrupt = socket_kernel_interrupt;

    cpu->cache = malloc(sizeof(CACHE));
    inicializarCACHE(cpu->cache);

    cpu->tlb = malloc(sizeof(TLB));
    inicializarTLB(cpu->tlb);

    pthread_mutex_init(&cpu->mutex_interrupcion, NULL);

    return cpu;
}

void cerrarCPU(cpu_t *args_cpu) {
    //////////////////// Libero Cache ////////////////////
    if(args_cpu->cache != NULL) {
        if(args_cpu->cache->entradas != NULL) {
            for(int i = 0; i < CACHE_SIZE; i++) {
                if(args_cpu->cache->entradas[i].contenido != NULL) {
                    free(args_cpu->cache->entradas[i].contenido);
                }
            }
            free(args_cpu->cache->entradas);
        }
        free(args_cpu->cache);
    }

    //////////////////// Libero TLB ////////////////////
    if (args_cpu->tlb != NULL) {
        if(args_cpu->tlb->entradas != NULL) {
            free(args_cpu->tlb->entradas);
        }
        free(args_cpu->tlb);
    }

    ///////////////////// Otros /////////////////////////
    list_destroy_and_destroy_elements(args_cpu->interrupciones, free);
    pthread_mutex_destroy(&args_cpu->mutex_interrupcion);
    free(args_cpu);
}

void liberarConexiones(int socket_memoria, int socket_kernel_dispatch, int socket_kernel_interrupt) {
    liberarConexion(socket_memoria);
    liberarConexion(socket_kernel_dispatch);
    liberarConexion(socket_kernel_interrupt);
}