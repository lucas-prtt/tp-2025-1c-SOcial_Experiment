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

void crear_PCB_cpu(PCB_cpu *proc_AEjecutar) { // Si despues necesita mas cosas del PCB se inician acá
    proc_AEjecutar = NULL;
}

enum TIPO_INSTRUCCION instrucciones_string_to_enum(char *nombreInstruccion) {
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
        devolverProcesoAlKernel(); //TODO
    }
    pthread_mutex_unlock(&mutexInterrupcion);
}