#include <instrucciones.h>


void *atenderKernelDispatch(void *cpu_args) {
    cpu_t *cpu = (cpu_t*)cpu_args;

    while(true) {
        PCB_cpu proc_AEjecutar;
        int estado_conexion = 0;
        
        if(!recibirPIDyPCkernel(cpu->socket_kernel_dispatch, &proc_AEjecutar, &estado_conexion)) {
            switch(estado_conexion)
            {
                case -1:
                {
                    log_debug(logger, "Modulo Kernel desconectado. Terminando hilo dispatch CPU");
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
            bool fin_proceso = ejecutarCicloInstruccion(cpu, &proc_AEjecutar);
                log_trace(logger, "Instruccion completada! Vamos a la siguiente?");
            if(fin_proceso) {
                log_trace(logger, "No vamos nada, me voy a tomar mate");
                limpiarProcesoTLB(cpu->tlb, proc_AEjecutar.pid);
                limpiarProcesoCACHE(cpu->socket_memoria, cpu->cache, proc_AEjecutar.pid);
                break;
            }
            log_trace(logger, "Sigamos trabajando! El que no hace overclock es porque no quiere");
        }
    }

    pthread_exit(NULL);
}

void *atenderKernelInterrupt(void *cpu_args) {
    cpu_t *cpu = (cpu_t*)cpu_args;

    int *pidInterruptor = malloc(sizeof(int));
    *pidInterruptor = recibirInterrupcion(cpu->socket_kernel_interrupt);
    while(*pidInterruptor != -1) {
        pthread_mutex_lock(&cpu->mutex_interrupcion);
        list_add(cpu->interrupciones, pidInterruptor);
        pthread_mutex_unlock(&cpu->mutex_interrupcion);
        log_info(logger, "## Llega interrupción al puerto Interrupt");
        pidInterruptor = malloc(sizeof(int));
        *pidInterruptor = recibirInterrupcion(cpu->socket_kernel_interrupt);
    }

    log_debug(logger, "Modulo Kernel desconectado o se recibio mensaje incorrecto. Terminando hilo interrupt CPU");
    pthread_exit(NULL);
}