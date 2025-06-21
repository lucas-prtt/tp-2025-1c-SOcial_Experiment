#include <instrucciones.h>


void *atenderKernelDispatch(void *cpu_args) {
    cpu_t *cpu = (cpu_t*)cpu_args;

    log_info(logger, "Hilo de Dispatch iniciado. Esperando procesos del Kernel...");

    while(true) {
        PCB_cpu proc_AEjecutar;
        int estado_conexion = 0;

        log_debug(logger, "Esperando nuevo proceso del Kernel (dispatch)...");
        
        if(!recibirPIDyPC_kernel(cpu->socket_kernel_dispatch, &proc_AEjecutar, &estado_conexion)) {
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
                default:
                    log_error(logger, "Fallo en recibirPIDyPC_kernel con código de estado desconocido: %d", estado_conexion);
            }
            break;
        }

        log_info(logger, "## Recibido proceso PID: %d con PC inicial: %d", proc_AEjecutar.pid, proc_AEjecutar.program_counter);
        
        for(;;) {
            log_debug(logger, "Ejecutando instrucción para PID %d en PC %d", proc_AEjecutar.pid, proc_AEjecutar.program_counter);
            bool fin_proceso = ejecutarCicloInstruccion(cpu, &proc_AEjecutar);
            if(fin_proceso) {
                log_info(logger, "Fin de ejecución para PID %d. Limpiando entradas de TLB.", proc_AEjecutar.pid);
                limpiarProcesoTLB(cpu->tlb, proc_AEjecutar.pid);
                break;
            }
        }
    }

    log_info(logger, "Hilo de Dispatch finalizado.");
    pthread_exit(NULL);
}

void *atenderKernelInterrupt(void *cpu_args) {
    cpu_t *cpu = (cpu_t*)cpu_args;
    
    log_info(logger, "Hilo de Interrupt iniciado. Esperando interrupciones del Kernel...");

    while(true) {
        if(recibirInterrupcion(cpu->socket_kernel_interrupt)) {
            pthread_mutex_lock(&cpu->mutex_interrupcion);
            cpu->hay_interrupcion = true;
            pthread_mutex_unlock(&cpu->mutex_interrupcion);

            log_info(logger, "## Llega interrupción al puerto Interrupt");
        } else {
            log_error(logger, "Fallo al recibir interrupción. Posible desconexión del Kernel. Terminando hilo interrupt.");
        }
    }

    log_info(logger, "Hilo de Interrupt finalizado.");
    pthread_exit(NULL);
}