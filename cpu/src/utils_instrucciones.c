#include "utils_instrucciones.h"



// VARIABLES GLOBALES:
bool hayInterrupcion = false;
pthread_mutex_t mutexInterrupcion; // MUTEX para acceder a hayInterrupcion



bool recibirPIDyPC_kernel(int socket_kernel_dispatch, PCB_cpu *proc_AEjecutar) {
    int *codigo_operacion = malloc(sizeof(int));
    t_list *lista_PIDyPC = recibir_paquete_lista(socket_kernel_dispatch, MSG_WAITALL, codigo_operacion);
    if (lista_PIDyPC == NULL || list_size(lista_PIDyPC) < 4 || *codigo_operacion != ASIGNACION_PROCESO_CPU) {
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

bool ejecutarCicloInstruccion(int socket_memoria, int socket_kernel, PCB_cpu *proc_AEjecutar) {
    char *instruccion = fetch(socket_memoria, proc_AEjecutar);
    instruccionInfo instr_info = decode(instruccion);
    bool fin_proceso = execute(socket_memoria, socket_kernel, instruccion, instr_info, proc_AEjecutar);
    //checkInterrupt();
    free(instruccion);

    return fin_proceso;
}

char *fetch(int socket_memoria, PCB_cpu *proc_AEjecutar) {
    // Pide una instrucccion a memoria a partir de proc_AEjecutar //
    t_paquete *paquete_peticion_instr = crear_paquete(PETICION_INSTRUCCION_MEMORIA);
    agregar_a_paquete(paquete_peticion_instr, &(proc_AEjecutar->pid), sizeof(proc_AEjecutar->pid));
    agregar_a_paquete(paquete_peticion_instr, &(proc_AEjecutar->pc), sizeof(proc_AEjecutar->pc));
    enviar_paquete(paquete_peticion_instr, socket_memoria);
    eliminar_paquete(paquete_peticion_instr);
    // Recibe la instruccion en forma de char* //
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

instruccionInfo decode(char *instruccion) {
    char *nombre_instruccion = devolverOperacion(instruccion);
    enum TIPO_INSTRUCCION tipo = instrucciones_string_to_enum(nombre_instruccion);
    instruccionInfo instr_info;
    instr_info.tipo_instruccion = tipo;
    instr_info.requiere_traduccion = false;
    if (tipo == INSTR_WRITE || tipo == INSTR_READ) {
        instr_info.requiere_traduccion = true;
    }

    free(nombre_instruccion);
    return instr_info;
}

char *devolverOperacion(char *instruccion) {
    // Duplica la cadena (además reserva dinamicamente) //
    char *copia_de_instruccion = strdup(instruccion);
    // Extrae la primera palabra de la cadena (nombre de la instrucción) //
    char *nombre_instruccion = strtok(copia_de_instruccion, " ");
    char *operacion = strdup(nombre_instruccion);

    free(copia_de_instruccion);
    return operacion;
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

bool execute(int socket_memoria, int socket_kernel, char *instruccion, instruccionInfo instr_info, PCB_cpu *pcb) {
    char *copia_de_instruccion = strdup(instruccion);
    char *operacion = strtok(copia_de_instruccion, " ");
    int direccion_fisica;

    if (instr_info.requiere_traduccion) {
        // Traduce la memoria logica a memoria fisica //
        int direccion_logica = atoi(strtok(NULL, " "));
        //direccion_fisica = transDeLogicaAFisica(direccion_logica); // TODO:
    }

    switch (instr_info.tipo_instruccion)
    {
        case INSTR_NOOP:
        {
            sleep(1); // Simula ciclo de CPU
            setProgramCounter(pcb, pcb->pc + 1);
            break;
        }
        case INSTR_WRITE:
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
        case INSTR_READ: // TODO: que pasa si hay eror al recibir la lista?
        {
            int tam = atoi(strtok(NULL, " "));

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
            char *valor = strtok(NULL, " ");
            setProgramCounter(pcb, atoi(valor));
            break;
        }
        case INSTR_IO:
        {
            char *dispositivo = strtok(NULL, " ");
            int tiempo = atoi(strtok(NULL, " "));

            t_paquete *paquete_peticion_io = crear_paquete(SYSCALL_IO);
            agregar_a_paquete(paquete_peticion_io, &(pcb->pc), sizeof(pcb->pc));
            agregar_a_paquete(paquete_peticion_io, dispositivo, sizeof(strlen(dispositivo) + 1));
            agregar_a_paquete(paquete_peticion_io, &tiempo, sizeof(tiempo));
            enviar_paquete(paquete_peticion_io, socket_kernel);
            setProgramCounter(pcb, pcb->pc + 1);

            eliminar_paquete(paquete_peticion_io);
            break;
        }
        case INSTR_INIT_PROC:
        {
            char *path = strtok(NULL, " ");
            int tamanio = atoi(strtok(NULL, " "));

            t_paquete *paquete_peticion_init_proc = crear_paquete(SYSCALL_INIT_PROC);
            agregar_a_paquete(paquete_peticion_init_proc, &(pcb->pc), sizeof(pcb->pc));
            agregar_a_paquete(paquete_peticion_init_proc, path, strlen(path) + 1);
            agregar_a_paquete(paquete_peticion_init_proc, &tamanio, sizeof(tamanio));
            enviar_paquete(paquete_peticion_init_proc, socket_kernel);
            setProgramCounter(pcb, pcb->pc + 1);

            eliminar_paquete(paquete_peticion_init_proc);
            break;
        }
        case INSTR_DUMP_MEMORY:
        {
            t_paquete *paquete_peticion_dump_memory = crear_paquete(SYSCALL_DUMP_MEMORY);
            agregar_a_paquete(paquete_peticion_dump_memory, &(pcb->pc), sizeof(pcb->pc));
            enviar_paquete(paquete_peticion_dump_memory, socket_kernel);
            setProgramCounter(pcb, pcb->pc + 1);

            eliminar_paquete(paquete_peticion_dump_memory);
            break;
        }
        case INSTR_EXIT:
        {
            t_paquete *paquete_instr_exit = crear_paquete(SYSCALL_EXIT);
            agregar_a_paquete(paquete_instr_exit, &(pcb->pc), sizeof(pcb->pc)); //
            enviar_paquete(paquete_instr_exit, socket_kernel);
            setProgramCounter(pcb, pcb->pc + 1);

            eliminar_paquete(paquete_instr_exit);
            free(copia_de_instruccion);
            return true;
        }
        default:
        {
            log_error(logger, "Instrucción no reconocida: %s", operacion); // error o algo //AUMENTA EL PC?
            break;
        }
    }

    free(copia_de_instruccion);
    return false;
}

void setProgramCounter(PCB_cpu *pcb, int newProgramCounter) {
    pcb->pc = newProgramCounter;
}

void controlarInterrupciones(void) {
    pthread_mutex_lock(&mutexInterrupcion);
    if (hayInterrupcion) {
        hayInterrupcion = false;
        pthread_mutex_unlock(&mutexInterrupcion);

        log_info(logger, "Interrupción activa: devuelvo el proceso al Kernel");
        // devolverProcesoAlKernel(); // TODO
    }
    pthread_mutex_unlock(&mutexInterrupcion);
}