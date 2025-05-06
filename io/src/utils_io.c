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

/*
cuando se conecte al kernel, tiene que quedar en espera. Como? una espera activa con WHILE? O nada que ver

La estructura del PCB sera algo como esto?
typedef struct {
    int PID
    int PC //arranca en 0
    ME //una lista
    MT //una lista
} PCB;

*/

/*
recibirPeticion(int socket_kernel, request_io &request, int n) {
    //verificar posibles errores
    t_list *lista_request = recibir_paquete_lista(socket_kernel, MSG_WAITALL, null); //null?
    request.pid = *(int*)list_get(lista_request, 1);
    request.tiempo = *(int*)list_get(lista_request, 3); //por ahora segundos 
}

void ejecutarPeticion(t_log *logger, request_io request) {
    log_info(logger,"## PID: %d - Inicio de IO - Tiempo: %d", request.pid, request.tiempo);
    sleep(request.tiempo);
    //if() - evaluar el caso de las desconexiones...
    log_info(logger, "## PID: %d - Fin de IO", request.pid);
}

void notificarFinPeticion(int socket_kernel, int motivo) {
    t_paquete *paquete_notif = crear_paquete(HANDSHAKE); //FIN_IO O DESCONEXION_IO
    agregar_a_paquete(paquete_notif, &motivo, sizeof(int));
    enviar_paquete(paquete_notif, socket_kernel);
    eliminar_paquete(paquete_notif);
}
*/

//ENUM { FIN_IO, DESCONEXION_IO }
