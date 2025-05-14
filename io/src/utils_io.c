#include "utils_io.h"


void cerrarIO(void) {
    //Cerrar log y config
    abort();
}

int handshakeKernel(int socket_kernel, char* nombre) { //Envia una solicitud de handshake al kernel//
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
        return -1;
    }
    int result = *(int*)list_get(lista_contenido, 1);
    free(codigo_operacion);
    eliminar_paquete(paquete_consult_cliente);
    eliminar_paquete_lista(lista_contenido);
    return result;
}

/*
int recibirPeticion(int socket_kernel, request_io &request) {
    int *codigo_operacion;
    t_list *lista_request = recibir_paquete_lista(socket_kernel, MSG_WAITALL, codigo_operacion);
    if(lista_request == NULL || list_size(lista_request) < 4 || get*(int*)codigo_operacion != PETICION_IO) {
        eliminar_paquete_lista(lista_request);
        return 0; //
    }
    request.pid = *(int*)list_get(lista_request, 1);
    request.tiempo = *(int*)list_get(lista_request, 3); //por ahora segundos
    eliminar_paquete_lista(lista_request);
    return 1; //
}

void ejecutarPeticion(t_log *logger, request_io request) {
    log_info(logger,"## PID: %d - Inicio de IO - Tiempo: %d", request.pid, request.tiempo);
    sleep(request.tiempo);
    //if() - evaluar el caso de las desconexiones...
    log_info(logger, "## PID: %d - Fin de IO", request.pid);
}

void notificarMotivoFinPeticion(int socket_kernel, MOTIVO_FIN_IO motivo) {
    t_paquete *paquete_notif = crear_paquete(n); //FIN_IO O DESCONEXION_IO, debe ser n un doigo de operacion
    agregar_a_paquete(paquete_notif, &motivo, sizeof(int));
    enviar_paquete(paquete_notif, socket_kernel);
    eliminar_paquete(paquete_notif);
}
*/


