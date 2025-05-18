#include "utils_instrucciones.h"


bool recibirPIDyPC_kernel(int socket_kernel_dispatch, PCB_cpu *proc_AEjecutar) {
    int *codigo_operacion = malloc(sizeof(int));
    t_list *lista_PIDyPC = recibir_paquete_lista(socket_kernel_dispatch, MSG_WAITALL, codigo_operacion);
    if(lista_PIDyPC == NULL || list_size(lista_PIDyPC) < 4 || *codigo_operacion != ASIGNACION_PROCESO_CPU) {
        free(codigo_operacion);
        eliminar_paquete_lista(lista_PIDyPC);
        return false;
    }
    proc_AEjecutar->pid = *(int*)list_get(lista_PIDyPC, 1);
    proc_AEjecutar->pc = *(int*)list_get(lista_PIDyPC, 3);
    free(codigo_operacion);
    eliminar_paquete_lista(lista_PIDyPC);
    return true;
}

void controlarInterrupciones(void) {
    pthread_mutex_lock(&mutexInterrupcion);
    if(hayInterrupcion) {
        hayInterrupcion = false;
        pthread_mutex_unlock(&mutexInterrupcion);

        log_info(logger, "Interrupción activa: devuelvo el proceso al Kernel");
        devolverProcesoAlKernel(); //TODO
    }
    pthread_mutex_unlock(&mutexInterrupcion);
}