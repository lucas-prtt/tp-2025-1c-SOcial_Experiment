#include "utils_io.h"


void cerrarIO(void) {
    cerrarConfigYLog();
    abort();
}

void verificarConexionKernel(int socket_cliente) {
    if(socket_cliente == -1)
        log_info(logger, "Conexión Inicial - Kernel - Error");
    else
        log_info(logger, "Conexión Inicial - Kernel - Success");
}

bool handshakeKernel(int socket_kernel, char* nombre) {
    int *codigo_operacion = malloc(sizeof(int));
    t_paquete* paquete_consult_cliente = crear_paquete(HANDSHAKE);
    agregar_a_paquete(paquete_consult_cliente, nombre, strlen(nombre)+1);
    log_debug(logger, "PAQUETE CONTIENE: ID:%d, TAM:%d, NOMBRE:%s, TAM_NOMBRE:%d", paquete_consult_cliente->tipo_mensaje, paquete_consult_cliente->tamanio, (char*)(paquete_consult_cliente->buffer + sizeof(int)), *((int*)(paquete_consult_cliente->buffer)));
    enviar_paquete(paquete_consult_cliente, socket_kernel);
    t_list *lista_contenido = recibir_paquete_lista(socket_kernel, MSG_WAITALL, codigo_operacion);
    if(lista_contenido == NULL || list_size(lista_contenido) < 2 || *codigo_operacion != HANDSHAKE || *(int*)list_get(lista_contenido, 1) == -1) {
        free(codigo_operacion);
        eliminar_paquete(paquete_consult_cliente);
        eliminar_paquete_lista(lista_contenido);
        return false;
    }
    bool result = false;
    char* respuesta = (char*)list_get(lista_contenido, 1);
    if(strcmp(respuesta, nombre) == 0) result = true;
    free(codigo_operacion);
    eliminar_paquete(paquete_consult_cliente);
    eliminar_paquete_lista(lista_contenido);
    return result;
}

void verificarResultadoHandshake_Kernel(bool result) {
    if(result)
        log_debug(logger, "Handshake - Kernel - Success");
    else
        log_error(logger, "Handshake - Kernel - Error");
}

bool recibirPeticion(int socket_kernel, request_io *request) {
    int *codigo_operacion;
    codigo_operacion = malloc(sizeof(int));
    t_list *lista_request = recibir_paquete_lista(socket_kernel, MSG_WAITALL, codigo_operacion);
    if(lista_request == NULL || *codigo_operacion != PETICION_IO) {
        free(codigo_operacion);
        if(lista_request != NULL) {
            eliminar_paquete_lista(lista_request);
        }
        return false;
    }
    request->pid = *(int*)list_get(lista_request, 1);
    request->tiempo = *(int*)list_get(lista_request, 3);
    if(lista_request) {
        log_debug(logger, "Request recibida (PID: %d, Tiempo: %d", request->pid, request->tiempo);
    }

    free(codigo_operacion);
    eliminar_paquete_lista(lista_request);
    return true;
}

void ejecutarPeticion(request_io *request, MOTIVO_FIN_IO *motivo) {
    log_info(logger,"## PID: %d - Inicio de IO - Tiempo: %d", request->pid, request->tiempo);
    usleep(request->tiempo*1000); // Son milisegundos, no microsegundos
    *motivo = IO_SUCCESS;
    log_info(logger, "## PID: %d - Fin de IO", request->pid);
}

void notificarMotivoFinPeticion(int socket_kernel, MOTIVO_FIN_IO motivo) {
    t_paquete *paquete_notif = crear_paquete(RESPUESTA_PETICION);
    agregar_a_paquete(paquete_notif, &motivo, sizeof(int));
    enviar_paquete(paquete_notif, socket_kernel);
    eliminar_paquete(paquete_notif);
}
