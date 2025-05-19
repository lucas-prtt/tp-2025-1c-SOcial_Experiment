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

void ejecutarInstruccion(int socket_memoria, PCB_cpu *proc_AEjecutar, bool *fin_ejecucion) {
    // CICLO DE INSTRUCCION //

    // FETCH: busca la instrucción a memoria //
    pedirInstruccionAMemoria(socket_memoria, proc_AEjecutar);
    t_list *instruccion = recibirInstruccionMemoria(socket_memoria); // Bloqueante: memoria le responde con un paquete en forma de lista (INSTRUCCION)

    // DECODE: interpreta la instruccion recibida como 'string' y la convierte en 'enum' //
    tipo_de_instruccion = interpretarInstruccion((char*)list_get(instruccion, 1));
    
    // EXECUTE: ejecuta la instrucción recibida //
    switch(tipo_de_instruccion)
    {
        case INSTR_NOOP:
        {
            //ejecutar_instruccion_noop();
            break;
        }
        case INSTR_WRITE:
        {
            //ejecutar_instruccion_write();
            break;
        }
        case INSTR_READ:
        {
            //ejecutar_instruccion_read();
            break;
        }
        case INSTR_GOTO:
        {
            //ejecutar_instruccion_goto();
            break;
        }
        case INSTR_IO:
        {
            //ejecutar_instruccion_io();
            break;
        }
        case INSTR_INIT_PROC:
        {
            //ejecutarInstruccion_init_proc();
            break;
        }
        case INSTR_DUMP_MEMORY:
        {
            //ejecutarInstruccion_dump_memory();
            break;
        }
        case INSTR_EXIT:
        {
            //ejecutarInstruccion_exit();
            break;
        }
        default:
        {
            //
            break;
        }
    }
    
    //TODO: liberar instruccion_recibida
    
    controlarInterrupciones();
}


void pedirInstruccionAMemoria(int socket_memoria, PCB_cpu *proc_AEjecutar) {
    t_paquete *paquete_peticion_instr = crear_paquete(PETICION_INSTRUCCION_MEMORIA);
    agregar_a_paquete(paquete_peticion_instr, &(proc_AEjecutar->pid), sizeof(proc_AEjecutar->pid));
    agregar_a_paquete(paquete_peticion_instr, &(proc_AEjecutar->pc), sizeof(proc_AEjecutar->pc));
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




char* fetch(int socket_memoria, int pid, int pc) {
    t_paquete* paquete = crear_paquete(FETCH_INSTRUCCION); //Creo que no hay un enum para decirle a memoria que voy a hacer un fetch
    agregar_a_paquete(paquete, &pid, sizeof(int));
    agregar_a_paquete(paquete, &pc, sizeof(int));
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);

    int* codigo_operacion = malloc(sizeof(int));
    t_list* lista = recibir_paquete_lista(socket_memoria, MSG_WAITALL, codigo_operacion);

    if(lista == NULL || *codigo_operacion != FETCH_INSTRUCCION || list_size(lista) < 2) {
        free(codigo_operacion);
        eliminar_paquete_lista(lista);
        return NULL;
    }

    char* instruccion = strdup((char*)list_get(lista, 1));
    free(codigo_operacion);
    eliminar_paquete_lista(lista);

    return instruccion;
}

tipoYconDireccion decode(char* instruccion, PCB_cpu* pcb) {
    
    char* copia_de_instruccion = strdup(instruccion);

    // Extraemos la primer palabra (el nombre de la instrucción)
    char* nombreInstr = strtok(instruccion_copy, " ");

    // Identificamos el tipo de instrucción
    enum TIPO_INSTRUCCION tipo = instrucciones_string_to_enum(nombreInstr);

    tipoYconDireccion tipoYconDir;
    tipoconDir.tipo = tipo;
    tipoconDir.esDireccion = false;
    if(tipo == INSTR_WRITE || tipo == INSTR_READ){
        tipoconDir.esDireccion = true;
    }

    free(instruccion_copy);
    return tipoYconDir;
}

void execute(char* instruccion, tipoYconDireccion tipoYconDir, PCB_cpu* pcb, int socket_memoria) {
    char* instr_copy = strdup(instruccion); /// Para no modificar la original
    char* operacion = strtok(instr_copy, " ");
    
    switch (tipoYconDir.tipo) {
        case INSTR_NOOP:
            sleep(1); // Simula ciclo de CPU
            break;

        case INSTR_WRITE: {
            char* dir_logica = strtok(NULL, " ");
            char* datos = strtok(NULL, " ");
            uint32_t dir_fisica = transDeLogicaAFisica(dir_logica);//falta hacerla

            t_paquete* paquete = crear_paquete(ESCRIBIR_EN_MEMORIA); //Crear Enum para escribir en memoria
            agregar_a_paquete(paquete, &pcb->pid, sizeof(int));
            agregar_a_paquete(paquete, &direccion_fisica, sizeof(int));
            agregar_a_paquete(paquete, datos, strlen(datos) + 1);
            enviar_paquete(paquete, socket_memoria);
            eliminar_paquete(paquete);
            break;
        }

        case INSTR_READ: {
            char* dir_logica = strtok(NULL, " ");
            char* tamanio = strtok(NULL, " ");
            uint32_t dir_fisica = transDeLogicaAFisica(dir_logica);//falta hacerla
            uint32_t tamanio = atoi(tamanio);

            t_paquete* paquete = crear_paquete(LEER_DE_MEMORIA); // Enum que debe existir
            agregar_a_paquete(paquete, &pid, sizeof(int));
            agregar_a_paquete(paquete, &direccion_logica, sizeof(int));
            agregar_a_paquete(paquete, &tamanio, sizeof(int));
            enviar_paquete(paquete, socket_memoria);
            eliminar_paquete(paquete);

            int* cod_op = malloc(sizeof(int));
            t_list* lista = recibir_paquete_lista(socket_memoria, MSG_WAITALL, cod_op);

            if (lista == NULL || *cod_op != LEER_DE_MEMORIA || list_size(lista) < 1) {
                free(cod_op);
                eliminar_paquete_lista(lista);
                return NULL;//deberia dar error o algo asi supongo? y deberia liberar instr_copy
            }

            char* leido = strdup((char*)list_get(lista, 0));
            free(cod_op);
            eliminar_paquete_lista(lista);
            break;
        }

        case INSTR_GOTO: {
            char* valor = strtok(NULL, " ");
            pcb->pc = atoi(valor);
            free(instr_copy);
            return; // Salgo para no sumar 1 al PC, porque ya fue actualizado
        }

        case INSTR_IO:// No se si en verdad deberia de encargarse de notificar que llamada es a kernel o se encarga el CPU
        case INSTR_INIT_PROC:
        case INSTR_DUMP_MEMORY:
        case INSTR_EXIT:
            t_paquete* paquete = crear_paquete(NOTIFICAR_SYSCALL_A_KERNELL); // Enum que debe existir
            agregar_a_paquete(paquete, &tipoYconDir.tipo, sizeof(enum TIPO_INSTRUCCION));
            agregar_a_paquete(paquete, &instruccion, strlen(instruccion) + 1);
            agregar_a_paquete(paquete, &pcb, sizeof(int));
            enviar_paquete(paquete, socket_kernel);//no se de donde lo sacaria
            eliminar_paquete(paquete);
            
            if (tipoDir.tipo == INSTR_EXIT) {
                free(instr_copy);
                return true;  // Finalizó ejecución
            }
            break;

        default:
            log_error(logger, "Instrucción no reconocida: %s", operacion);//error o algo
            break;
    }

    pcb->pc++; // Actualizamos PC si no fue GOTO
    free(instr_copy);
}
