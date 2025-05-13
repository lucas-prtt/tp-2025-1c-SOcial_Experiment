#include "utils_cpu.h"

t_log* iniciarLogger(char* nombreArchivo, char* nombreProceso, t_log_level logLevel) {
    t_log* nuevoLogger = log_create(nombreArchivo, nombreProceso, false, logLevel);
    if(nuevoLogger == NULL) { abort(); }
    return nuevoLogger;
}

t_config* iniciarConfig(char *nombreArchivoConfig) {
    t_config* nuevoConfig = config_create(nombreArchivoConfig);
    if(nuevoConfig == NULL) { abort(); }
    return nuevoConfig;
}

void cerrarCPU(void) {
    //cerrar log y config
    abort();
}

int handshakeClient(int socket_cliente, int identificador) { //Envia una solicitud de handshake al modulo X//
    int *codigo_operacion = malloc(sizeof(int));
    t_paquete* paquete_consult_cliente = crear_paquete(HANDSHAKE);
    agregar_a_paquete(paquete_consult_cliente, &identificador, sizeof(int));
    enviar_paquete(paquete_consult_cliente, socket_cliente);
    t_list *lista_contenido = recibir_paquete_lista(socket_cliente, MSG_WAITALL, codigo_operacion);
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