#include "utils_cpu.h"


void cerrarCPU(void) {
    cerrarConfigYLog();
    abort();
}

void verificarConexionCliente(int socket_cliente, char* nombreModuloCliente) {
    if(socket_cliente == -1)
        log_info(logger, "%s - Conexión Inicial - Error", nombreModuloCliente);
    else
        log_info(logger, "%s - Conexión Inicial - Exito", nombreModuloCliente);
}

bool handshakeClient(int socket_cliente, int identificador) { //Envia una solicitud de handshake al modulo X//
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