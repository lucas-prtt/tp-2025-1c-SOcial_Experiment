#include "utils_cpu.h"


void cerrarCPU(void) {
    cerrarConfigYLog();
    abort();
}

int generarSocket(char* ip_cliente, char* puerto_cliente, char* modulo_cliente) {
    int socket = conectarSocketClient(ip_cliente, puerto_cliente);
    verificarConexionCliente(socket, modulo_cliente);
    return socket;
}

void verificarConexionCliente(int socket_cliente, char* nombreModuloCliente) { //TODO
    if(socket_cliente == -1)
        log_info(logger, "%s - Conexión Inicial - Error", nombreModuloCliente);
    else
        log_info(logger, "%s - Conexión Inicial - Exito", nombreModuloCliente);
}

void realizarHandshake(int socket_cliente, int identificadorCPU, char* modulo_cliente) {
    bool resultHandshake = handshakeCliente(socket_cliente, identificadorCPU);
    verificarResultadoHandshake(resultHandshake, modulo_cliente);
}

bool handshakeCliente(int socket_cliente, int identificador) {
    int *codigo_operacion = malloc(sizeof(int));
    t_paquete* paquete_consult_cliente = crear_paquete(HANDSHAKE);
    agregar_a_paquete(paquete_consult_cliente, &identificador, sizeof(int));
    enviar_paquete(paquete_consult_cliente, socket_cliente);
    t_list *lista_contenido = recibir_paquete_lista(socket_cliente, MSG_WAITALL, codigo_operacion);
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

void verificarResultadoHandshake(bool result, char* nombreModuloCliente) {
    if(result)
        log_info(logger, "%s Handshake - Exito", nombreModuloCliente);
    else
        log_info(logger, "%s Handshake - Error", nombreModuloCliente);
}

sockets_dispatcher *prepararSocketsDispatcher(int socket_memoria, int socket_kernel_dispatch) {
    sockets_dispatcher *sockets_d = malloc(sizeof(sockets_dispatcher));
    sockets_d->socket_memoria = socket_memoria;
    sockets_d->socket_kernel_dispatch = socket_kernel_dispatch;
    return sockets_d;
}