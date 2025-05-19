#include <instrucciones.h>



void *atenderKernelDispatch(void *sockets) {
    int socket_memoria = ((sockets_dispatcher*)sockets)->socket_memoria;
    int socket_kernel_dispatch = ((sockets_dispatcher*)sockets)->socket_kernel_dispatch;

    while(true) {
        PCB_cpu proc_AEjecutar;
        if(recibirPIDyPC_kernel(socket_kernel_dispatch, &proc_AEjecutar)) {
            bool fin_ejecucion = false;
            //preparar_PCB_cpu(&proc_AEjecutar);
            while(!fin_ejecucion)
                ejecutarInstruccion(socket_memoria, &proc_AEjecutar, &fin_ejecucion); //TODO: CAMBIAR fin_ejecucion dentro de ejecutarInstruccion
        }
        else break; //TODO
    }
    free(sockets);
    return NULL; //TODO
}

void *atenderKernelInterrupt(void *socket) {
    int socket_kernel_interrupt = *(int*)socket;
    while(true) {
        //TODO
        /*if(recibirInterrupcion(socket_kernel_interrupt)) {
            pthread_mutex_lock(&mutexInterrupcion);
            hayInterrupcion = true;
            pthread_mutex_unlock(&mutexInterrupcion);

            log_info(logger, "## Llega interrupción al puerto Interrupt");
        }*/
    }
}
