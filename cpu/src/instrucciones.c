#include <instrucciones.h>



void *atenderKernelDispatch(void *cpu_args) {
    cpu_t *cpu = (cpu_t*)cpu_args;
    int socket_memoria = cpu->socket_memoria;
    int socket_kernel_dispatch = cpu->socket_kernel_dispatch;

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
            bool fin_proceso = ejecutarCicloInstruccion(socket_memoria, socket_kernel_dispatch, &proc_AEjecutar, cpu);
            if(fin_proceso) break;
            //devolverProcesoAlKernel(&proc_AEjecutar, FIN_PROCESO);
            /*Una vez seleccionado el proceso a ejecutar... y se enviará a uno de los módulos CPU... quedando a la espera de recibir dicho PID y el PC después de
            la ejecución junto con un motivo por el cual fue devuelto

            hay motivos de devolucion...
            */
        }
    }

    pthread_exit(NULL);
}

void *atenderKernelInterrupt(void *cpu_args) {
    cpu_t *cpu = (cpu_t*)cpu_args;
    int socket_kernel_interrupt = cpu->socket_kernel_interrupt;
    
    while(true) {
        if(recibirInterrupcion(socket_kernel_interrupt)) {
            pthread_mutex_lock(&cpu->mutex_interrupcion);
            cpu->hay_interrupcion = true;
            pthread_mutex_unlock(&cpu->mutex_interrupcion);

            log_info(logger, "## Llega interrupción al puerto Interrupt");
        }
    }

    pthread_exit(NULL);
}