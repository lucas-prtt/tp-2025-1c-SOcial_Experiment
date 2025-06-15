#include "utils_instrucciones.h"


bool recibirPIDyPC_kernel(int socket_kernel_dispatch, PCB_cpu *proc_AEjecutar, int *estado_conexion) {
    int *codigo_operacion = malloc(sizeof(int));
    t_list *lista_PIDyPC = recibir_paquete_lista(socket_kernel_dispatch, MSG_WAITALL, codigo_operacion);

    if(lista_PIDyPC == NULL) {
        *estado_conexion = -1; // Desconexión //
        free(codigo_operacion);
        return false;
    }
    else if(list_size(lista_PIDyPC) < 4 || *codigo_operacion != ASIGNACION_PROCESO_CPU) {
        *estado_conexion = -2; // Error al recibir el paquete //
        free(codigo_operacion);
        eliminar_paquete_lista(lista_PIDyPC);
        return false;
    }
    
    proc_AEjecutar->pid = *(int *)list_get(lista_PIDyPC, 1);
    proc_AEjecutar->pc = *(int *)list_get(lista_PIDyPC, 3);

    free(codigo_operacion);
    eliminar_paquete_lista(lista_PIDyPC);
    return true;
}

bool ejecutarCicloInstruccion(cpu_t *cpu, PCB_cpu *proc_AEjecutar) {
    instruccionInfo instr_info;

    char *instruccion = fetch(cpu->socket_memoria, proc_AEjecutar);
    log_info(logger, "## PID: %d - FETCH - Program Counter: %d", proc_AEjecutar->pid, proc_AEjecutar->pc);

    t_list *instruccion_list = decode(instruccion, &instr_info);
    bool fin_proceso = execute(cpu->socket_memoria, cpu->socket_kernel_dispatch, instruccion_list, instr_info, proc_AEjecutar);
    log_info(logger, "## PID: %d - Ejecutando: %s - <PARAMETROS>",  proc_AEjecutar->pid, (char *)list_get(instruccion_list, 0));
    if(checkInterrupt(cpu)) {
        devolverProcesoKernel(cpu->socket_kernel_dispatch, proc_AEjecutar);
        free(instruccion);
        list_destroy(instruccion_list);
        return true;
    }
    
    free(instruccion);
    list_destroy(instruccion_list);
    return fin_proceso;
}

char *fetch(int socket_memoria, PCB_cpu *proc_AEjecutar) { //Funciona en casos de CACHE_MISS y TLB_MISS
    // Pide una instrucccion a memoria a partir de proc_AEjecutar //
    t_paquete *paquete_peticion_instr = crear_paquete(PETICION_INSTRUCCION_MEMORIA);
    agregar_a_paquete(paquete_peticion_instr, &(proc_AEjecutar->pid), sizeof(proc_AEjecutar->pid));
    agregar_a_paquete(paquete_peticion_instr, &(proc_AEjecutar->pc), sizeof(proc_AEjecutar->pc));
    enviar_paquete(paquete_peticion_instr, socket_memoria);
    eliminar_paquete(paquete_peticion_instr);
    
    int *codigo_operacion;
    codigo_operacion = malloc(sizeof(int));
    t_list *lista_respuesta = recibir_paquete_lista(socket_memoria, MSG_WAITALL, codigo_operacion);
    if (lista_respuesta == NULL || list_size(lista_respuesta) < 2 || *codigo_operacion != RESPUESTA_INSTRUCCION_MEMORIA) {
        free(codigo_operacion);
        eliminar_paquete_lista(lista_respuesta);
        return NULL;
    }
    char *instruccion = strdup((char *)list_get(lista_respuesta, 1));

    free(codigo_operacion);
    eliminar_paquete_lista(lista_respuesta);
    return instruccion;
}

t_list *decode(char *instruccion, instruccionInfo *instr_info) {
    // Crear lista para los parámetros //
    t_list *instruccion_decodificada = list_create();

    // Duplica la cadena para manipulación //
    char *copia_de_instruccion = strdup(instruccion);
    char *token = strtok(copia_de_instruccion, " ");  // Primer token (operación)

    // Decodificar tipo de instrucción //
    instr_info->tipo_instruccion = instrucciones_string_to_enum(token);
    instr_info->requiere_traduccion = (instr_info->tipo_instruccion == INSTR_WRITE || instr_info->tipo_instruccion == INSTR_READ);
    list_add(instruccion_decodificada, strdup(token));

    // Agregar los parámetros restantes a la lista //
    while ((token = strtok(NULL, " ")) != NULL) {
        list_add(instruccion_decodificada, strdup(token));  // Copia del token
    }

    free(copia_de_instruccion);  // Liberar memoria duplicada
    return instruccion_decodificada;
}

enum TIPO_INSTRUCCION instrucciones_string_to_enum(char *nombreInstruccion) {
    if (!strcmp(nombreInstruccion, "NOOP")) {
        return INSTR_NOOP;
    }
    if (!strcmp(nombreInstruccion, "WRITE")) {
        return INSTR_WRITE;
    }
    if (!strcmp(nombreInstruccion, "READ")) {
        return INSTR_READ;
    }
    if (!strcmp(nombreInstruccion, "GOTO")) {
        return INSTR_GOTO;
    }
    if (!strcmp(nombreInstruccion, "IO")) {
        return INSTR_IO;
    }
    if (!strcmp(nombreInstruccion, "INIT_PROC")) {
        return INSTR_INIT_PROC;
    }
    if (!strcmp(nombreInstruccion, "DUMP_MEMORY")) {
        return INSTR_DUMP_MEMORY;
    }
    if (!strcmp(nombreInstruccion, "EXIT")) {
        return INSTR_EXIT;
    }
    return ERROR_NO_INSTR;
}

bool execute(int socket_memoria, int socket_kernel, t_list *instruccion_list, instruccionInfo instr_info, PCB_cpu *pcb) {
    char *operacion = (char *)list_get(instruccion_list, 0);
    int direccion_fisica = -1;

    if(instr_info.requiere_traduccion) {
        direccion_fisica = traducirDireccionCACHE(); // TODO:
        if(direccion_fisica == -1) {
            //direccion_fisica = traducirDireccionTLB(cpu, pcb->pid, atoi(list_get(instruccion_list, 1)));
            //cpu por el tamano de la pagina, capaz me covniene que ese tamano se global
        }
    }

    switch (instr_info.tipo_instruccion)
    {
        case INSTR_NOOP:
        {
            sleep(1); // Simula ciclo de CPU
            setProgramCounter(pcb, pcb->pc + 1);
            break;
        }
        case INSTR_WRITE: //
        {
            char* datos = strtok(NULL, " ");

            t_paquete *paquete_peticion_write = crear_paquete(PETICION_ESCRIBIR_EN_MEMORIA);
            agregar_a_paquete(paquete_peticion_write, &pcb->pid, sizeof(int));
            agregar_a_paquete(paquete_peticion_write, &direccion_fisica, sizeof(int));
            agregar_a_paquete(paquete_peticion_write, datos, strlen(datos) + 1);
            enviar_paquete(paquete_peticion_write, socket_memoria);
            setProgramCounter(pcb, pcb->pc + 1);

            eliminar_paquete(paquete_peticion_write);
            break;
        }
        case INSTR_READ: //
        {
            int tam = atoi(strtok(NULL, " ")); // cambio

            t_paquete *paquete_peticion_read = crear_paquete(PETICION_LEER_DE_MEMORIA);
            agregar_a_paquete(paquete_peticion_read, &pcb->pid, sizeof(int));
            agregar_a_paquete(paquete_peticion_read, &direccion_fisica, sizeof(int));
            agregar_a_paquete(paquete_peticion_read, &tam, sizeof(int));
            enviar_paquete(paquete_peticion_read, socket_memoria);
            eliminar_paquete(paquete_peticion_read);

            int *cod_op = malloc(sizeof(int));
            t_list *lista = recibir_paquete_lista(socket_memoria, MSG_WAITALL, cod_op);
            if(lista == NULL || *cod_op != RESPUESTA_PETICION || list_size(lista) < 1) {
                free(cod_op);
                eliminar_paquete_lista(lista);
                // break; // Como deberia manejarlo? Deberia salir y avanzar a la siguiente instruccion?
            }
            // char *leido = strdup((char *)list_get(lista, 0)); // No se usa
            setProgramCounter(pcb, pcb->pc + 1);

            free(cod_op);
            eliminar_paquete_lista(lista);
            break;
        }
        case INSTR_GOTO:
        {
            char *valor = atoi((char *)list_get(instruccion_list, 1));
            setProgramCounter(pcb, atoi(valor));
            break;
        }
        case INSTR_IO:
        {
            char *dispositivo = (char *)list_get(instruccion_list, 1);
            int tiempo = atoi((char *)list_get(instruccion_list, 2));

            t_paquete *paquete_peticion_io = crear_paquete(SYSCALL_IO);
            agregar_a_paquete(paquete_peticion_io, &(pcb->pc), sizeof(pcb->pc));
            agregar_a_paquete(paquete_peticion_io, dispositivo, sizeof(strlen(dispositivo) + 1));
            agregar_a_paquete(paquete_peticion_io, &tiempo, sizeof(tiempo));
            enviar_paquete(paquete_peticion_io, socket_kernel);
            setProgramCounter(pcb, pcb->pc + 1);

            eliminar_paquete(paquete_peticion_io);
            return true; // break;
        }
        case INSTR_INIT_PROC:
        {
            char *path = (char *)list_get(instruccion_list, 1);
            int tamanio = atoi((char *)list_get(instruccion_list, 2));

            t_paquete *paquete_peticion_init_proc = crear_paquete(SYSCALL_INIT_PROC);
            agregar_a_paquete(paquete_peticion_init_proc, &(pcb->pc), sizeof(pcb->pc));
            agregar_a_paquete(paquete_peticion_init_proc, path, strlen(path) + 1);
            agregar_a_paquete(paquete_peticion_init_proc, &tamanio, sizeof(tamanio));
            enviar_paquete(paquete_peticion_init_proc, socket_kernel);
            setProgramCounter(pcb, pcb->pc + 1);

            eliminar_paquete(paquete_peticion_init_proc);
            return true; // break;
        }
        case INSTR_DUMP_MEMORY:
        {
            t_paquete *paquete_peticion_dump_memory = crear_paquete(SYSCALL_DUMP_MEMORY);
            agregar_a_paquete(paquete_peticion_dump_memory, &(pcb->pc), sizeof(pcb->pc));
            enviar_paquete(paquete_peticion_dump_memory, socket_kernel);
            setProgramCounter(pcb, pcb->pc + 1);

            eliminar_paquete(paquete_peticion_dump_memory);
            return true; // break;
        }
        case INSTR_EXIT:
        {
            t_paquete *paquete_instr_exit = crear_paquete(SYSCALL_EXIT);
            agregar_a_paquete(paquete_instr_exit, &(pcb->pc), sizeof(pcb->pc)); //
            enviar_paquete(paquete_instr_exit, socket_kernel);
            setProgramCounter(pcb, pcb->pc + 1);

            eliminar_paquete(paquete_instr_exit);

            // Devolver proceso al kernel //

            return true;
        }
        default:
        {
            log_error(logger, "Instrucción no reconocida: %s", operacion); // error o algo //AUMENTA EL PC?
            break;
        }
    }

    return false;
}

int traducirDireccionCACHE(void) {}

int traducirDireccionTLB(cpu_t *cpu, int pid, int direccion_logica) { // capaz me deberia devolver -1 si esta deshabilitada, para que el resolver con memoria no este muy duplicaod
    int nro_pagina = getNumeroPagina(direccion_logica);
    int desplazamiento = getDesplazamiento(direccion_logica);
    int marco = -1;

    if(cpu->tlb->habilitada) {
        marco = buscarPaginaTLB(cpu->tlb, pid, nro_pagina);

        if(marco == -1) {
            marco = resolver_pagina_con_memoria(pid, nro_pagina); // ke
            reemplazarEnTLB(cpu->tlb, nro_pagina, marco);
        }
    }
    else {
        marco = resolver_pagina_con_memoria(pid, nro_pagina); // ke
    }

    return marco * tamanio_pagina + desplazamiento;
}

void setProgramCounter(PCB_cpu *pcb, int newProgramCounter) {
    pcb->pc = newProgramCounter;
}



/////////////////////////       < INTERRUPCIONES >       /////////////////////////

bool recibirInterrupcion(int socket_kernel_dispatch) { //preguntar a lucas
    int *codigo_operacion = malloc(sizeof(int));
    t_list *lista_interrupcion = recibir_paquete_lista(socket_kernel_dispatch, MSG_WAITALL, codigo_operacion);

    if(lista_interrupcion == NULL || ist_size(lista_interrupcion) < 2 || *codigo_operacion != PETICION_INTERRUPT_A_CPU) {
        free(codigo_operacion);
        return false;
    }

    free(codigo_operacion);
    eliminar_paquete_lista(lista_interrupcion);
    return true;
}


bool checkInterrupt(cpu_t *cpu) {
    pthread_mutex_lock(&cpu->mutex_interrupcion);
    if(cpu->hay_interrupcion) {
        cpu->hay_interrupcion = false;
        pthread_mutex_unlock(&cpu->mutex_interrupcion);

        log_debug(logger, "Interrupción activa: devuelvo el proceso al Kernel");
        return true;
    }

    return false;
}

void devolverProcesoAlKernel(int socket_kernel, PCB_cpu *proc_AEjecutar) {
    t_paquete *paquete_devolucion_proceso = crear_paquete(INTERRUPT_ACKNOWLEDGE);
    agregar_a_paquete(paquete_devolucion_proceso, &(proc_AEjecutar->pc), sizeof(proc_AEjecutar->pc));
    enviar_paquete(paquete_devolucion_proceso, socket_kernel);

    eliminar_paquete(paquete_devolucion_proceso);
}