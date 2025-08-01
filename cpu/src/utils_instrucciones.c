#include "utils_instrucciones.h"

bool ultimaInstruccionInitProc;

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
    instruccionInfo instr_info;

    char *instruccion = fetch(cpu->socket_memoria, proc_AEjecutar);
    t_list *instruccion_list = decode(proc_AEjecutar, instruccion, &instr_info);
    log_trace(logger, "Estoy a punto de hacer execute()");
    bool fin_proceso = execute(cpu, instruccion_list, instr_info, proc_AEjecutar);
    bool interrupcion = checkInterrupt(cpu, proc_AEjecutar);
    log_trace(logger, "Ejecute! Fin proceso: %d, interrupt: %d, initProc: %d", fin_proceso, interrupcion, ultimaInstruccionInitProc);


    free(instruccion);
    list_destroy_and_destroy_elements(instruccion_list, free);
    
    if(fin_proceso) {
        log_trace(logger, "Interrumpo ejecucion, le mando un mensaje a kernel");
        log_trace(logger, "Aquí me voy!");
        if(interrupcion && ultimaInstruccionInitProc) { // Si a kernel le hice syscall de INIT_PROC, automaticamente me manda el mismo proceso.
            t_list * ConfirmacionSyscallRecibido = recibir_paquete_lista(cpu->socket_kernel_dispatch, MSG_WAITALL, NULL);
                    // No tengo que aceptar el mismo proceso que me mande. Le debo dar interrupt acknoledge.
            eliminar_paquete_lista(ConfirmacionSyscallRecibido);
            log_debug(logger, "Le digo a kernel que me llego interrupcion del proceso, por lo que no lo tiene que seguir ejecutando");
            limpiarProcesoCACHE(cpu, proc_AEjecutar->pid);
            devolverProcesoPorInterrupt(cpu->socket_kernel_dispatch, proc_AEjecutar);
            ultimaInstruccionInitProc = false;
        }else{
            if(interrupcion){
                log_debug(logger, "hubo interrupcion y syscall, pero no era init_proc, por lo que al volver, la cola se ordena y no hace falta interrupt acknowledge");
            }
            else if(ultimaInstruccionInitProc){
                log_debug(logger, "Hubo initProc pero sin interrupcion, kernel me va a mandar que siga con este proceso");
            }
            else{
                log_debug(logger, "Se produjo un syscall normal, sin interrupciones presentes");
            }
        }
        return true;
    }else if(interrupcion){
        log_debug(logger, "Solo hubo una interrupcion, no hubo syscall. Devuelvo interrupt acknoledge");
        // Si solo hubo interrupcion, solo devuelvo interrupt acknowledge
        limpiarProcesoCACHE(cpu, proc_AEjecutar->pid);
        devolverProcesoPorInterrupt(cpu->socket_kernel_dispatch, proc_AEjecutar);
        ultimaInstruccionInitProc = false;  // Ya deberia ser, pero por las dudas
        return true;
    }
    ultimaInstruccionInitProc = false;

    log_trace(logger, "Sigo ejecutando");
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
    //MostrameTodaLaCache(cpu);
    ultimaInstruccionInitProc = false;
    switch(instr_info.tipo_instruccion)
    {
        case INSTR_NOOP: // No se devuelve a Kernel
        {
            log_info(logger, "## PID: %d - Ejecutando: %s", pcb->pid, operacion);
            setProgramCounter(pcb, pcb->pc + 1);
            return false;
        }
        case INSTR_WRITE: //No se devuelve a Kernel
        {
            int direccion_logica = atoi((char*)list_get(instruccion_list, 1));
            char *datos = (char *)list_get(instruccion_list, 2);
            int tamanio_datos = strlen(datos);

            char *datos2 = malloc(tamanio_datos + 1);
            memcpy(datos2, datos, tamanio_datos);
            datos2[tamanio_datos] = '\0';
            if(cpu->cache->habilitada) {
                // WRITE en cache //
                escribirEnCache(cpu, pcb->pid, direccion_logica, datos2);
            }
            else {
                // WRITE en memoria //
                escribirEnMemoria(cpu, pcb->pid, direccion_logica, datos2);
            }
            free(datos2);
            log_info(logger, "## PID: %d - Ejecutando: %s - Dirección: %d - Datos: %s", pcb->pid, operacion, direccion_logica, datos);
            setProgramCounter(pcb, pcb->pc + 1);
            return false;
        }
        case INSTR_READ: // No se devuelve a Kernel
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
        case INSTR_GOTO:    //No se devuelve a Kernel
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
            limpiarProcesoCACHE(cpu, pcb->pid);
            t_paquete *paquete_peticion_io = crear_paquete(SYSCALL_IO);
            setProgramCounter(pcb, pcb->pc + 1);
            agregar_a_paquete(paquete_peticion_io, &(pcb->pc), sizeof(pcb->pc));
            agregar_a_paquete(paquete_peticion_io, dispositivo, strlen(dispositivo) + 1);
            agregar_a_paquete(paquete_peticion_io, &tiempo, sizeof(tiempo));
            agregar_a_paquete(paquete_peticion_io, &(pcb->pid), sizeof(int));
            enviar_paquete(paquete_peticion_io, socket_kernel);
            eliminar_paquete(paquete_peticion_io);

            log_info(logger, "## PID: %d - Ejecutando: %s - Dispositivo: %s - Tiempo: %d", pcb->pid, operacion, dispositivo, tiempo);
            return true;
        }
        case INSTR_INIT_PROC:
        {
            ultimaInstruccionInitProc = true;
            char *path = (char *)list_get(instruccion_list, 1);
            int tamanio = atoi((char *)list_get(instruccion_list, 2));
            limpiarProcesoCACHE(cpu, pcb->pid);
            t_paquete *paquete_peticion_init_proc = crear_paquete(SYSCALL_INIT_PROC);
            setProgramCounter(pcb, pcb->pc + 1);
            agregar_a_paquete(paquete_peticion_init_proc, &(pcb->pc), sizeof(pcb->pc));
            agregar_a_paquete(paquete_peticion_init_proc, path, strlen(path) + 1);
            agregar_a_paquete(paquete_peticion_init_proc, &tamanio, sizeof(tamanio));
            agregar_a_paquete(paquete_peticion_init_proc, &(pcb->pid), sizeof(int));
            enviar_paquete(paquete_peticion_init_proc, socket_kernel);
            eliminar_paquete(paquete_peticion_init_proc);

            log_info(logger, "## PID: %d - Ejecutando: %s - Archivo de instrucciones: %s - Tamaño: %d", pcb->pid, operacion, path, tamanio);
            return true;
        }
        case INSTR_DUMP_MEMORY:
        {
            limpiarProcesoCACHE(cpu, pcb->pid);
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
            limpiarProcesoCACHE(cpu, pcb->pid);
            t_paquete *paquete_instr_exit = crear_paquete(SYSCALL_EXIT);
            setProgramCounter(pcb, pcb->pc + 1);
            agregar_a_paquete(paquete_instr_exit, &(pcb->pc), sizeof(pcb->pc));
            agregar_a_paquete(paquete_instr_exit, &(pcb->pid), sizeof(int));
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
            if(!cache->entradas[i].bit_modificado) {
                cache->entradas[i].bit_modificado = 1;
            }
            break;
        }
    }
}

void setProgramCounter(PCB_cpu *pcb, int newProgramCounter) {
    pcb->pc = newProgramCounter;
}



/////////////////////////       < INTERRUPCIONES >       /////////////////////////

int recibirInterrupcion(int socket_kernel_interrupt) {
    int *codigo_operacion = malloc(sizeof(int));
    t_list *lista_interrupcion = recibir_paquete_lista(socket_kernel_interrupt, MSG_WAITALL, codigo_operacion);
    log_debug(logger, "Se recibio paquete en puerto interrupt");
    if(lista_interrupcion == NULL) {
        log_debug(logger, "Modulo Kernel desconectado. Terminando hilo interrupt CPU");
        free(codigo_operacion);
        return -1;
    }
    if(list_size(lista_interrupcion) < 2 || *codigo_operacion != PETICION_INTERRUPT_A_CPU) {
        log_error(logger, "Me mandaron cualquier cosa en el parquete de Interrupt");
        log_error(logger, "Pointer = %p", lista_interrupcion);
        log_error(logger, "Tamaño = %d, CodOp = %d", list_size(lista_interrupcion), *codigo_operacion);
        free(codigo_operacion);
        return -1;
    }
    int pid = *(int*)list_get(lista_interrupcion, 1);
    log_debug(logger, "Paquete de interrupt correcto");
    log_debug(logger, "El paquete de interrupcion corresponde al proceso %d", pid);
    free(codigo_operacion);
    eliminar_paquete_lista(lista_interrupcion);
    return pid;
}

bool checkInterrupt(cpu_t *cpu, PCB_cpu *proc_AEjecutar) {
    pthread_mutex_lock(&cpu->mutex_interrupcion);
    while(list_size(cpu->interrupciones) > 0){
        int * pidInterruptor = (int*)list_remove(cpu->interrupciones, 0);
        if(*pidInterruptor == proc_AEjecutar->pid){
        pthread_mutex_unlock(&cpu->mutex_interrupcion);
        list_clean_and_destroy_elements(cpu->interrupciones, free);
        free(pidInterruptor);
        log_debug(logger, "Interrupción activa");
        return true;
        }
        else{
            //log_warning(logger, "Falsa alarma. Se recibio un interrupt de otro proceso (%d) en lugar del que se esta ejecutando (%d)", *pidInterruptor, proc_AEjecutar->pid);
            log_debug(logger, "Sigo buscando interrupciones");
            list_clean_and_destroy_elements(cpu->interrupciones, free);
        }
    }
    pthread_mutex_unlock(&cpu->mutex_interrupcion);
    log_debug(logger, "No hay interrupciones para el preoceso %d", proc_AEjecutar->pid);
    return false;
}

void devolverProcesoPorInterrupt(int socket_kernel, PCB_cpu *proc_AEjecutar) {
    t_paquete *paquete_devolucion_proceso = crear_paquete(INTERRUPT_ACKNOWLEDGE);
    
    log_trace(logger, "El mensaje dice: <El proceso que se estaba ejecutando quedo en PC: %d>", proc_AEjecutar->pc);
    agregar_a_paquete(paquete_devolucion_proceso, &(proc_AEjecutar->pc), sizeof(proc_AEjecutar->pc));
    agregar_a_paquete(paquete_devolucion_proceso, &(proc_AEjecutar->pid), sizeof(int));
    enviar_paquete(paquete_devolucion_proceso, socket_kernel);

    eliminar_paquete(paquete_devolucion_proceso);
}

void MostrameTodaLaCache(cpu_t * cpu){
    char * buffer = NULL;
    buffer = malloc(tamanio_pagina+1);   
    buffer[tamanio_pagina] = '\0';
    for(int i=0; i<CACHE_SIZE; i++){
        if(cpu->cache->entradas[i].contenido != NULL){
        memcpy(buffer, cpu->cache->entradas[i].contenido, tamanio_pagina);
        log_warning(logger, "%s", buffer);}
    }
}
