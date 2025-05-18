#include <instrucciones.h>



void *atenderKernelDispatch(void *socket) {
    int socket_kernel_dispatch = *(int*)socket;
    while(true) {
        PCB_cpu proc_AEjecutar;
        if(recibirPIDyPC_kernel(socket_kernel_dispatch, &proc_AEjecutar)) {
            bool fin_ejecucion = false;

            preparar_PCB_cpu(&proc_AEjecutar);
            while(!fin_ejecucion)
                ejecutarInstruccion(&proc_AEjecutar, fin_ejecucion); //TODO: CAMBIAR fin_ejecucion dentro de ejecutarInstruccion
        }
        else break; //TODO
    }
    free(socket);
    return NULL; //TODO
}

void *atenderKernelInterrupt(void *socket) {
    int socket_kernel_interrupt = *(int*)socket;
    while(true) {
        if(recibirInterrupcion(socket_kernel_interrupt)) { //TODO
            pthread_mutex_lock(&mutexInterrupcion);
            hayInterrupcion = true;
            pthread_mutex_unlock(&mutexInterrupcion);

            log_info(logger, "## Llega interrupción al puerto Interrupt");
        }
    }
}
