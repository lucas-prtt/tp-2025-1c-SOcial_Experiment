#include <instrucciones.h>



void *atenderKernelDispatch(void *sockets) {
    int socket_memoria = ((sockets_dispatcher*)sockets)->socket_memoria;
    int socket_kernel_dispatch = ((sockets_dispatcher*)sockets)->socket_kernel_dispatch;

    while(true) {
        PCB_cpu proc_AEjecutar;
        int estado_conexion = 0;

        if(!recibirPIDyPC_kernel(socket_kernel_dispatch, &proc_AEjecutar, &estado_conexion)) {
            switch(estado_conexion)
            {
                case -1:
                {
                    log_info(logger, "Modulo Kernel desconectado. Terminando hilo dispatch CPU");
                    break;
                }          
                case -2:
                {
                    log_error(logger, "Se recibió un mensaje mal formado desde Kernel. Ignorando.");
                    continue;
                }
            }
            break;
        }
        
        for(;;) {
            bool fin_proceso = ejecutarCicloInstruccion(socket_memoria, socket_kernel_dispatch, &proc_AEjecutar);
            if(fin_proceso) break;
        }
    }

    free(sockets);
    pthread_exit(NULL);
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
