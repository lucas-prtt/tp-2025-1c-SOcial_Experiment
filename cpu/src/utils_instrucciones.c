#include "utils_instrucciones.h"



// VARIABLES GLOBALES:
bool hayInterrupcion = false;
pthread_mutex_t mutexInterrupcion;      // MUTEX para acceder a hayInterrupcion



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

void preparar_PCB_cpu(PCB_cpu *proc_AEjecutar) {}



void pedirInstruccionAMemoria(int socket_memoria, int programCounter) {
    t_paquete *paquete_peticion_instr = crear_paquete(PETICION_INSTRUCCION_MEMORIA);
    agregar_a_paquete(paquete_peticion_instr, &programCounter, sizeof(programCounter));
    enviar_paquete(paquete_peticion_instr, socket_memoria);
    eliminar_paquete(paquete_peticion_instr);
}

t_list *recibirInstruccionMemoria(int socket_memoria) {
    int *codigo_operacion;
    codigo_operacion = malloc(sizeof(codigo_operacion));
    t_list *instruccion = recibir_paquete_lista(socket_memoria, MSG_WAITALL, codigo_operacion);
    if(instruccion == NULL || *codigo_operacion != RESPUESTA_PETICION) {} //TODO
    free(codigo_operacion);
    return instruccion;
}

enum TIPO_INSTRUCCION interpretarInstruccion(char *nombreInstruccion) {
    if(!strcmp(nombreInstruccion, "NOOP")) {
        return INSTR_NOOP;
    }
    if(!strcmp(nombreInstruccion, "WRITE")) {
        return INSTR_WRITE;
    }
    if(!strcmp(nombreInstruccion, "READ")) {
        return INSTR_READ;
    }
    if(!strcmp(nombreInstruccion, "GOTO")) {
        return INSTR_GOTO;
    }
    if(!strcmp(nombreInstruccion, "IO")) {
        return INSTR_IO;
    }
    if(!strcmp(nombreInstruccion, "INIT_PROC")) {
        return INSTR_INIT_PROC;
    }
    if(!strcmp(nombreInstruccion, "DUMP_MEMORY")) {
        return INSTR_DUMP_MEMORY;
    }
    if(!strcmp(nombreInstruccion, "EXIT")) {
        return INSTR_EXIT;
    }
    return ERROR_NO_INSTR;
}

void controlarInterrupciones(void) {
    pthread_mutex_lock(&mutexInterrupcion);
    if(hayInterrupcion) {
        hayInterrupcion = false;
        pthread_mutex_unlock(&mutexInterrupcion);

        log_info(logger, "Interrupción activa: devuelvo el proceso al Kernel");
        //devolverProcesoAlKernel(); //TODO
    }
    pthread_mutex_unlock(&mutexInterrupcion);
}