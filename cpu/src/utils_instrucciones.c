#include "utils_instrucciones.h"


bool recibirPIDyPCkernel(int socket_kernel_dispatch, PCB_cpu *proc_AEjecutar, int *estado_conexion) {
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
    log_debug(logger, "Instrucción recibida - PID: %d - Program Counter: %d", proc_AEjecutar->pid,  proc_AEjecutar->pc);

    free(codigo_operacion);
    eliminar_paquete_lista(lista_PIDyPC);
    return true;
}

bool ejecutarCicloInstruccion(cpu_t *cpu, PCB_cpu *proc_AEjecutar) {
    log_trace(logger, "EjecutarCicloInstruccion()");
    instruccionInfo instr_info;

    char *instruccion = fetch(cpu->socket_memoria, proc_AEjecutar);
    t_list *instruccion_list = decode(proc_AEjecutar, instruccion, &instr_info);
    log_trace(logger, "Estoy a punto de hacer execute(), preparense todos!");
    bool fin_proceso = execute(cpu, instruccion_list, instr_info, proc_AEjecutar);
    log_trace(logger, "PUMBA!!! ejecute!. fin_proceso = %d", fin_proceso);

    if(checkInterrupt(cpu, proc_AEjecutar) || fin_proceso) {
        log_trace(logger, "Hubo un problema, las veré pronto mis instrucciones queridas");
        log_trace(logger, "Le mando un mensaje a kernel");
        free(instruccion);
        list_destroy_and_destroy_elements(instruccion_list, free);
        log_trace(logger, "Aquí me voy!");
        return true;
    }

    log_trace(logger, "El mundo es hermoso. Sigo ejecutando");
    free(instruccion);
    list_destroy_and_destroy_elements(instruccion_list, free);
    return false;
}

char *fetch(int socket_memoria, PCB_cpu *proc_AEjecutar) {
    t_paquete *paquete_peticion_instr = crear_paquete(PETICION_INSTRUCCION_MEMORIA);
    agregar_a_paquete(paquete_peticion_instr, &(proc_AEjecutar->pid), sizeof(proc_AEjecutar->pid));
    agregar_a_paquete(paquete_peticion_instr, &(proc_AEjecutar->pc), sizeof(proc_AEjecutar->pc));
    enviar_paquete(paquete_peticion_instr, socket_memoria);
    eliminar_paquete(paquete_peticion_instr);
    
    int *codigo_operacion;
    codigo_operacion = malloc(sizeof(int));
    t_list *lista_respuesta = recibir_paquete_lista(socket_memoria, MSG_WAITALL, codigo_operacion);
    if(lista_respuesta == NULL  || *codigo_operacion != RESPUESTA_INSTRUCCION_MEMORIA) {
        free(codigo_operacion);
        eliminar_paquete_lista(lista_respuesta);
        return NULL;
    }

    char *instruccion = strdup((char *)list_get(lista_respuesta, 1));
    log_info(logger, "## PID: %d - FETCH - Program Counter: %d", proc_AEjecutar->pid, proc_AEjecutar->pc);

    free(codigo_operacion);
    eliminar_paquete_lista(lista_respuesta);
    return instruccion;
}

t_list *decode(PCB_cpu *proc_AEjecutar, char *instruccion, instruccionInfo *instr_info) {
    t_list *instruccion_decodificada = list_create();
    char *copia_de_instruccion = strdup(instruccion);
    char *token = strtok(copia_de_instruccion, " ");  // Primer token: Operación //
    log_debug(logger, "## PID: %d - DECODE - Program Counter: %d - Instrucción: %s", proc_AEjecutar->pid, proc_AEjecutar->pc, token);

    instr_info->tipo_instruccion = instrucciones_string_to_enum(token);
    instr_info->requiere_traduccion = (instr_info->tipo_instruccion == INSTR_WRITE || instr_info->tipo_instruccion == INSTR_READ);
    list_add(instruccion_decodificada, strdup(token));

    while ((token = strtok(NULL, " ")) != NULL) {
        list_add(instruccion_decodificada, strdup(token));
    }

    free(copia_de_instruccion);
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

bool execute(cpu_t *cpu, t_list *instruccion_list, instruccionInfo instr_info, PCB_cpu *pcb) {
    int socket_kernel = cpu->socket_kernel_dispatch;
    char *operacion = (char *)list_get(instruccion_list, 0);
    
    switch(instr_info.tipo_instruccion)
    {
        case INSTR_NOOP:
        {
            log_info(logger, "## PID: %d - Ejecutando: %s", pcb->pid, operacion);
            setProgramCounter(pcb, pcb->pc + 1);
            return false;
        }
        case INSTR_WRITE:
        {
            int direccion_logica = atoi((char*)list_get(instruccion_list, 1));
            char *datos = (char *)list_get(instruccion_list, 2);
            int tamanio_datos = strlen(datos);

            char *datos2 = malloc(tamanio_datos + 1);
            memcpy(datos2, datos, tamanio_datos);
            datos2[tamanio_datos] = '\0';
            log_debug(logger, "Me pidieron que escriba %s en %d", datos2, direccion_logica);
            if(cpu->cache->habilitada) {
                // WRITE en cache //
                escribirEnCache(cpu, pcb->pid, direccion_logica, datos2);
            }
            else {
                // WRITE en memoria //
                escribirEnMemoria(cpu, pcb->pid, direccion_logica, datos2);
            }

            log_info(logger, "## PID: %d - Ejecutando: %s - Dirección: %d - Datos: %s", pcb->pid, operacion, direccion_logica, datos);
            setProgramCounter(pcb, pcb->pc + 1);
            return false;
        }
        case INSTR_READ:
        {
            int direccion_logica = atoi((char*)list_get(instruccion_list, 1));
            int tamanio = atoi((char *)list_get(instruccion_list, 2));

            if(cpu->cache->habilitada) {
                // READ desde cache //
                leerDeCache(cpu, pcb->pid, direccion_logica, tamanio);
            }
            else {
                leerDeMemoria(cpu, pcb->pid, direccion_logica, tamanio);
            }

            log_info(logger, "## PID: %d - Ejecutando: %s - Direccion: %d - Tamaño: %d", pcb->pid, operacion, direccion_logica, tamanio);
            setProgramCounter(pcb, pcb->pc + 1);
            return false;
        }
        case INSTR_GOTO:
        {
            int valor = atoi((char *)list_get(instruccion_list, 1));

            log_info(logger, "## PID: %d - Ejecutando: %s - Valor: %d", pcb->pid, operacion, valor);
            setProgramCounter(pcb, valor);
            return false;
        }
        case INSTR_IO:
        {
            char *dispositivo = (char *)list_get(instruccion_list, 1);
            int tiempo = atoi((char *)list_get(instruccion_list, 2));

            t_paquete *paquete_peticion_io = crear_paquete(SYSCALL_IO);
            setProgramCounter(pcb, pcb->pc + 1);
            agregar_a_paquete(paquete_peticion_io, &(pcb->pc), sizeof(pcb->pc));
            agregar_a_paquete(paquete_peticion_io, dispositivo, strlen(dispositivo) + 1);
            agregar_a_paquete(paquete_peticion_io, &tiempo, sizeof(tiempo));
            enviar_paquete(paquete_peticion_io, socket_kernel);
            eliminar_paquete(paquete_peticion_io);

            log_info(logger, "## PID: %d - Ejecutando: %s - Dispositivo: %s - Tiempo: %d", pcb->pid, operacion, dispositivo, tiempo);
            return true;
        }
        case INSTR_INIT_PROC:
        {
            char *path = (char *)list_get(instruccion_list, 1);
            int tamanio = atoi((char *)list_get(instruccion_list, 2));

            t_paquete *paquete_peticion_init_proc = crear_paquete(SYSCALL_INIT_PROC);
            setProgramCounter(pcb, pcb->pc + 1);
            agregar_a_paquete(paquete_peticion_init_proc, &(pcb->pc), sizeof(pcb->pc));
            agregar_a_paquete(paquete_peticion_init_proc, path, strlen(path) + 1);
            agregar_a_paquete(paquete_peticion_init_proc, &tamanio, sizeof(tamanio));
            enviar_paquete(paquete_peticion_init_proc, socket_kernel);
            eliminar_paquete(paquete_peticion_init_proc);

            log_info(logger, "## PID: %d - Ejecutando: %s - Archivo de instrucciones: %s - Tamaño: %d", pcb->pid, operacion, path, tamanio);
            return true;
        }
        case INSTR_DUMP_MEMORY:
        {
            limpiarProcesoCACHETrucho(cpu->socket_memoria, cpu->cache, pcb->pid);
            t_paquete *paquete_peticion_dump_memory = crear_paquete(SYSCALL_DUMP_MEMORY);
            setProgramCounter(pcb, pcb->pc + 1);
            agregar_a_paquete(paquete_peticion_dump_memory, &(pcb->pc), sizeof(pcb->pc));
            enviar_paquete(paquete_peticion_dump_memory, socket_kernel);
            eliminar_paquete(paquete_peticion_dump_memory);

            log_info(logger, "## PID: %d - Ejecutando: %s", pcb->pid, operacion);
            return true;
        }
        case INSTR_EXIT:
        {
            t_paquete *paquete_instr_exit = crear_paquete(SYSCALL_EXIT);
            setProgramCounter(pcb, pcb->pc + 1);
            agregar_a_paquete(paquete_instr_exit, &(pcb->pc), sizeof(pcb->pc));
            enviar_paquete(paquete_instr_exit, socket_kernel);
            eliminar_paquete(paquete_instr_exit);

            log_info(logger, "## PID: %d - Ejecutando: %s", pcb->pid, operacion);
            return true;
        }
        default:
        {
            log_error(logger, "Instrucción no reconocida: %s", operacion);
            break;
        }
    }

    return false;
}

void marcarModificadoEnCache(CACHE *cache, int pid, int nro_pagina) {
    for(int i = 0; i < CACHE_SIZE; i++) {
        r_CACHE entrada = cache->entradas[i];
        if(entrada.pid == pid && entrada.pagina == nro_pagina) {
            cache->entradas[i].bit_modificado = 1;
            return;
        }
    }
}

void setProgramCounter(PCB_cpu *pcb, int newProgramCounter) {
    pcb->pc = newProgramCounter;
}



/////////////////////////       < INTERRUPCIONES >       /////////////////////////

bool recibirInterrupcion(int socket_kernel_interrupt) {
    int *codigo_operacion = malloc(sizeof(int));
    t_list *lista_interrupcion = recibir_paquete_lista(socket_kernel_interrupt, MSG_WAITALL, codigo_operacion);
    log_debug(logger, "Se recibio paquete en puerto interrupt");
    if(lista_interrupcion == NULL) {
        log_debug(logger, "Modulo Kernel desconectado. Terminando hilo interrupt CPU");
        free(codigo_operacion);
        return false;
    }
    if(list_size(lista_interrupcion) < 2 || *codigo_operacion != PETICION_INTERRUPT_A_CPU) {
        log_error(logger, "Me mandaron cualquier cosa en el parquete de Interrupt");
        log_error(logger, "Pointer = %p", lista_interrupcion);
        log_error(logger, "Tamaño = %d, CodOp = %d", list_size(lista_interrupcion), *codigo_operacion);
        free(codigo_operacion);
        return false;
    }
    log_debug(logger, "Paquete de interrupt correcto");
    free(codigo_operacion);
    eliminar_paquete_lista(lista_interrupcion);
    return true;
}

bool checkInterrupt(cpu_t *cpu, PCB_cpu *proc_AEjecutar) {
    pthread_mutex_lock(&cpu->mutex_interrupcion);
    if(cpu->hay_interrupcion) {
        cpu->hay_interrupcion = false;
        pthread_mutex_unlock(&cpu->mutex_interrupcion);
        
        log_debug(logger, "Interrupción activa: devuelvo el proceso al Kernel");
        devolverProcesoPorInterrupt(cpu->socket_kernel_dispatch, proc_AEjecutar);
        return true;
    }
    pthread_mutex_unlock(&cpu->mutex_interrupcion);

    return false;
}

void devolverProcesoPorInterrupt(int socket_kernel, PCB_cpu *proc_AEjecutar) {
    t_paquete *paquete_devolucion_proceso = crear_paquete(INTERRUPT_ACKNOWLEDGE);
    
    log_trace(logger, "El mensaje dice: <El proceso que se estaba ejecutando quedo en PC: %d>", proc_AEjecutar->pc);
    agregar_a_paquete(paquete_devolucion_proceso, &(proc_AEjecutar->pc), sizeof(proc_AEjecutar->pc));
    enviar_paquete(paquete_devolucion_proceso, socket_kernel);

    eliminar_paquete(paquete_devolucion_proceso);
}