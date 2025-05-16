#include <instrucciones.h>

/*
void atenderPeticionKernel(void) {
    while(true) {
        PIDyPC *proc_AEjecutar = recibirPIDyPC_kernel();
        bool fin_ejecucion = false;

        while(proc_AEjecutar && !fin_ejecucion) {
            ejecutarInstruccion(proc_AEjecutar, fin_ejecucion); //que cambie fin_ejecucion?
        }
        
    }
}

PIDyPC *recibirPIDyPC_kernel(int socket_kernel) { //y si iniciamos el socket aca adentro? En revision
    int *codigo_operacion = malloc(sizeof(int));
    t_list *lista_PIDyPC = recibir_paquete_lista(socket_kernel, MSG_WAITALL, codigo_operacion);
    if(lista_PIDyPC || list_size(lista_PIDyPC) < 4 || *codigo_operacion != PETICION_EXECUTE) {
        free(codigo_operacion);
        eliminar_paquete_lista(LISTA_PIDyPC);
        return NULL;
    }
    PIDyPC x = malloc(sizeof(PIDyPC));
    x.pid = *(int*)list_get(lista_PIDyPC, 1);
    x.pc = *(int*)list_get(lista_PIDyPC, 3);

    free(codigo_operacion);
    eliminar_paquete_lista(lista_PIDyPC);
    return x;
}

void ejecutarInstruccion(PIDyPC proc_AEjecutar, bool fin_ejecucion) {
    pedirInstruccionAMemoria(proc_AEjecutar->pc); //y sI inicio memoria aca?
    instr = recibirInstruccionMemoria()

    TIPO_INSTRUCCION tipoInstr = interpretarInstruccion(proc_AEjecutar); //devuelve el tipo de instruccion

    switch(tipoInstr) {
        case INSTR_NOOP:

        case INSTR_WHITE:
        
        case INSTR_READ:
        
        case INSTR_GOTO:
    
        case INSTR_IO:
        
        case INSTR_INIT_PROC:
        
        case INSTR_DUMP_MEMORY:
        
        case INSTR_EXIT:
    }
}

//traducirDireccionLogica

//actualizarProgramCounter

*/