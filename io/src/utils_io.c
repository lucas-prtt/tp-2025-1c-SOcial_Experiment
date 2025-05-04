#include "utils_io.h"


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

int handshakeKernel(int socket_kernel, int nombreIO) { //Envia una solicitud de handshake al modulo Kernel//
    int *codigo_operacion = malloc(sizeof(int));
    t_paquete* paquete_consult_kernel = crear_paquete(HANDSHAKE);
    agregar_a_paquete(paquete_consult_kernel, &nombreIO, sizeof(int));
    enviar_paquete(paquete_consult_kernel, socket_kernel);
    t_list *lista_contenido = recibir_paquete_lista(socket_kernel, MSG_WAITALL, codigo_operacion);
    if(lista_contenido == NULL || list_size(lista_contenido) < 2 || *codigo_operacion != HANDSHAKE || *(int*)list_get(lista_contenido, 1) == -1) {
        free(codigo_operacion);
        eliminar_paquete(paquete_consult_kernel);
        eliminar_paquete_lista(lista_contenido);
        return -1;
    }
    int result = *(int*)list_get(lista_contenido, 1);

    free(codigo_operacion);
    eliminar_paquete(paquete_consult_kernel);
    eliminar_paquete_lista(lista_contenido);
    return result;
}


//void verificarConexionInciales(int v, )

/*
cuando se conecte al kernel, tiene que quedar en espera. Como? una espera activa con WHILE? O nada que ver

La estructura del PCB sera algo como esto?
typedef struct {
    int PID
    int PC //arranca en 0
    ME //una lista
    MT //una lista
} PCB;

    invento:
typedef struct {
    int PID
    X timepo
} request;

recibirPeticion(PID?, tiempo?) { //recibe una request del kernel
    carga una request en una estructura de request? (la de arriba)
}

ejecutarPeticion() { //recibe la request de la funcion dearriba tal vez?
    //hace un sleep por el tiempo indicado por la request
    /ejecuta la peticion (sera el sleep?)
    //le informa al kernel que finalizo. Pienso en enviarle un:
    ENUM { FINIO, DESCONEXIONIO }
}
*/