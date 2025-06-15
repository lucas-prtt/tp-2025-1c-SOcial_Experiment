#include "utils_cpu.h"


/////////////////////////       < SOCKETS >       /////////////////////////

int generarSocket(char* ip_cliente, char* puerto_cliente, char* modulo_cliente) {
    int socket = conectarSocketClient(ip_cliente, puerto_cliente);
    verificarConexionCliente(socket, modulo_cliente);
    return socket;
}

void verificarConexionCliente(int socket_cliente, char* nombreModuloCliente) {
    if(socket_cliente == -1) {
        log_error(logger, "%s - Conexión Inicial - Error", nombreModuloCliente);
        exit(EXIT_FAILURE);
    }
    else
        log_debug(logger, "%s - Conexión Inicial - Exito", nombreModuloCliente);
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
    
    eliminar_paquete_lista(lista_contenido);
    return true;
}



/////////////////////////       < HANDSHAKE - KERNEL >       /////////////////////////

void realizarHandshakeKernel(int socket_cliente, int identificadorCPU, char* modulo_cliente) {
    bool resultHandshake = handshakeKernel(socket_cliente, identificadorCPU);
    verificarResultadoHandshake(resultHandshake, modulo_cliente);
}

bool handshakeKernel(int socket_kernel, int identificador) {
    int *codigo_operacion = malloc(sizeof(int));
    t_paquete* paquete_consult_cliente = crear_paquete(HANDSHAKE);
    agregar_a_paquete(paquete_consult_cliente, &identificador, sizeof(int));
    enviar_paquete(paquete_consult_cliente, socket_kernel);
    t_list *lista_contenido = recibir_paquete_lista(socket_kernel, MSG_WAITALL, codigo_operacion);
    if(lista_contenido == NULL || list_size(lista_contenido) < 2 || *codigo_operacion != HANDSHAKE || *(int*)list_get(lista_contenido, 1) == -1) {
        free(codigo_operacion);
        eliminar_paquete(paquete_consult_cliente);
        eliminar_paquete_lista(lista_contenido);
        return false;
    }
    int result = false;
    if(*(int*)list_get(lista_contenido, 1) == identificador) result = true;
    free(codigo_operacion);
    eliminar_paquete(paquete_consult_cliente);
    eliminar_paquete_lista(lista_contenido);
    return result;
}



/////////////////////////       < HANDSHAKE - UTILS >       /////////////////////////

void verificarResultadoHandshake(bool result, char* nombreModuloCliente) {
    if(result)
        log_debug(logger, "%s Handshake - Exito", nombreModuloCliente);
    else {
        log_error(logger, "%s Handshake - Error", nombreModuloCliente);
        exit(EXIT_FAILURE);
    }
}



/////////////////////////       < OTROS >       /////////////////////////

void cerrarCPU(void) {
    cerrarConfigYLog();
    abort();
}

cpu_t *prepararCPU(int socket_memoria, int socket_kernel_dispatch, int socket_kernel_interrupt) {
    cpu_t *cpu = malloc(sizeof(cpu));
    cpu->socket_memoria = socket_memoria;
    cpu->socket_kernel_dispatch = socket_kernel_dispatch;
    cpu->socket_kernel_interrupt = socket_kernel_interrupt;

    cpu->tlb = malloc(sizeof(TLB));
    inicializarTLB(cpu->tlb);

    cpu->hay_interrupcion = false;
    pthread_mutex_init(&cpu->mutex_interrupcion, NULL);

    return cpu;
}

void liberarConexiones(int socket_memoria, int socket_kernel_dispatch, int socket_kernel_interrupt) {
    liberarConexion(socket_memoria);
    liberarConexion(socket_kernel_dispatch);
    liberarConexion(socket_kernel_interrupt);
}