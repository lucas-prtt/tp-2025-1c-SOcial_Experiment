#include <instrucciones.h>



// VARIABLES GLOBALES: 
bool hayInterrupcion = false;
pthread_mutex_t mutexInterrupcion; // MUTEX para acceder a hayInterrupcion



void *atenderKernelDispatch(void *socket) {
    int socket_kernel_dispatch = *(int*)socket;
    while(true) {
        PIDyPC_instr proc_AEjecutar;
        if(recibirPIDyPC_kernel(socket_kernel_dispatch, &proc_AEjecutar)) {
            bool fin_ejecucion = false;
            while(!fin_ejecucion)
                ejecutarInstruccion(&proc_AEjecutar, fin_ejecucion); //TODO: CAMBIAR fin_ejecucion dentro de ejecutarInstruccion
        }
        else break; //TODO
    }
    free(socket);
    return NULL; //TODO
}

bool recibirPIDyPC_kernel(int socket_kernel_dispatch, PIDyPC_instr *proc_AEjecutar) {
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
        devolverProcesoAlKernel(); //Falta implementar
    }
    pthread_mutex_unlock(&mutexInterrupcion);
}

void *atenderKernelInterrupt(void *socket) {
    int socket_kernel_interrupt = *(int*)socket;
    while(true) {
        if(recibirInterrupcion(socket_kernel_interrupt)) { //No implementada por ahora
            pthread_mutex_lock(&mutexInterrupcion);
            hayInterrupcion = true;
            pthread_mutex_unlock(&mutexInterrupcion);

            log_info(logger, "## Llega interrupción al puerto Interrupt");
        }
    }
}
