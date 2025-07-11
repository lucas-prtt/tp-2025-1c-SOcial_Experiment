#include "procesos.h"


// VARIABLES GLOBALES: 
t_list * listasProcesos[7];      // Vector de lista para guardar procesos
int last_PID = 0;                // Estaba en 1
int qProcesosMolestando = 1;     // Cantidad de procesos que quedan ejecutar
///////////////////////////////////

// SEMAFOROS:
pthread_mutex_t mutex_listasProcesos    // MUTEX para interactuar con listasProcesos[7]
 = PTHREAD_MUTEX_INITIALIZER        ;   // 
pthread_mutex_t mutex_last_PID          // MUTEX para acceder a last_PID
 = PTHREAD_MUTEX_INITIALIZER        ;   // 
pthread_mutex_t mutex_procesos_molestando    // Mutex para acceder a qProcesosMolestando
 = PTHREAD_MUTEX_INITIALIZER        ;       //
                                        // Los mutex podrian ser particulares de cada elemento de la lista en algunos casos, pero es complicarse de más
sem_t sem_procesos_en_ready;            // Semeforo contador: Cantidad de procesos en READY
sem_t sem_ordenar_cola_ready;           // Representa peticion para ordenar la cola de ready y luego hacer signal a procesos en cola de ready (ya que se ejecuta al entrar un proceso nuevo)
sem_t sem_introducir_proceso_a_ready;   // Semaforo binario: Intenta mandar un proceso a memoria y a ready
//////////////////////////////////////////

void procesos_c_inicializarVariables(){
    sem_init(&sem_procesos_en_ready, 0, 0);
    sem_init(&sem_ordenar_cola_ready, 0, 0);
    sem_init(&sem_introducir_proceso_a_ready, 0, 0);
    for(int i = 0; i<7; i++){
        listasProcesos[i] = list_create();
    }    
    for(int i = 0; i<list_size(conexiones.IOEscucha); i++){
        sem_init(&((*(NombreySocket_IO*)list_get(conexiones.IOEscucha, i)).sem_peticiones), 0, 0);
        (*(NombreySocket_IO*)list_get(conexiones.IOEscucha, i)).cola = list_create();
    }
}
void * dispatcherThread(void * IDYSOCKETDISPATCH){ // Maneja la mayor parte de la interaccion con las CPU a traves del socket dispatch
    IDySocket_CPU * cpu = (IDySocket_CPU*) IDYSOCKETDISPATCH;
    int codOp;
    t_list * paqueteRespuesta;
    t_paquete * paqueteEnviado;
    t_PCB * proceso;
    int continuar_mismo_proceso;
    float alfa = atof(config_get_string_value(config, "ALFA"));

    while(1) {
        {  // Extraer proceso de lista de READY, pasarlo a EXEC
            int a; 
            sem_getvalue(&sem_procesos_en_ready, &a);
            log_debug(logger, "Procesos en ready segun semaforo: %d", a);
            sem_wait(&sem_procesos_en_ready);
            log_trace(logger, "Extrayendo proceso de la cola de ready.");
            pthread_mutex_lock(&mutex_listasProcesos);
            proceso = list_get(listasProcesos[READY], 0);
            cambiarEstado_EstadoActualConocido(proceso->PID, READY, EXEC, listasProcesos);
            proceso->ProcesadorQueLoEjecuta = cpu;
            pthread_mutex_unlock(&mutex_listasProcesos);
            log_debug(logger, "Se eligio el proceso (%d) para ejecutar", proceso->PID);
        }
        do{
            
            continuar_mismo_proceso = 0;
            paqueteEnviado = crear_paquete(ASIGNACION_PROCESO_CPU);
            agregar_a_paquete(paqueteEnviado, &(proceso->PID), sizeof(proceso->PID));
            agregar_a_paquete(paqueteEnviado, &(proceso->PC), sizeof(proceso->PC));
            enviar_paquete(paqueteEnviado, cpu->SOCKET);
            eliminar_paquete(paqueteEnviado);
            log_trace(logger, "Se asigno el proceso (%d) a la cpu %d en PC %d", proceso->PID, cpu->ID, proceso->PC);
            log_trace(logger, "El hilo de CPU %d se bloquea esperando respuesta", cpu->ID);
            
            paqueteRespuesta = recibir_paquete_lista(cpu->SOCKET, MSG_WAITALL, &codOp);
            log_trace(logger, "Se recibio un paquete de respuesta de proceso (%d) de la cpu %d", proceso->PID, cpu->ID);
            
            if (paqueteRespuesta == NULL){ // Si se cierra la conexion con el CPU, se cierra el hilo y se termina el proceso
                pthread_mutex_lock(&mutex_listasProcesos);
                cambiarEstado_EstadoActualConocido(proceso->PID, EXEC, EXIT, listasProcesos);
                proceso->ProcesadorQueLoEjecuta = NULL;
                pthread_mutex_unlock(&mutex_listasProcesos);
                liberarMemoria(proceso->PID);
                log_error(logger, "(%d) - Finaliza el proceso. Conexion con CPU (%d) perdida", proceso->PID, cpu->ID);
                pthread_exit(NULL);
            }
            int nuevoPC = *(int*)list_get(paqueteRespuesta, 1);
            log_trace(logger, "El parquete dice: Codop: %d,  proceso (%d) de la cpu %d PC paso a %d",codOp ,  proceso->PID, cpu->ID, *(int*)list_get(paqueteRespuesta, 1));
            proceso->PC = nuevoPC;

            // Las metricas (MT y ME) se actualizan solas en cambiarDeEstado()
            // cambiarDeEstado() tambien maneja los inicios y finalizaciones de los "timers" para cada estado y actualiza Ejecucion actual
            // Los auxiliares de Estimacion, Ejecucion actual y anterior se actualizan con actualizarEstimacion()
            // actualizarEstimacion debe ejecutarse despues de cambiarDeEstado()
            // Se podria hacer que no se ejecute actualizarEstimacion si el algoritmo es FIFO

            if (codOp != INTERRUPT_ACKNOWLEDGE){
                log_info(logger, "## (%d) - Solicitó syscall: %s", proceso->PID, syscallAsString(codOp));

            }
            switch (codOp)//TODO: Cada caso con su logica: En funcion de codOp se hace cada syscall
            {
            case SYSCALL_EXIT:
                log_trace(logger, "Ejecuto case EXIT");
                pthread_mutex_lock(&mutex_listasProcesos);
                cambiarEstado_EstadoActualConocido(proceso->PID, EXEC, EXIT, listasProcesos);
                log_trace(logger, "Se va el proceso (%d) a exit", proceso->PID);
                sem_post(&evaluarFinKernel);
                proceso->ProcesadorQueLoEjecuta = NULL;
                pthread_mutex_unlock(&mutex_listasProcesos);
                liberarMemoria(proceso->PID); // Envia mensaje a Memoria para liberar el espacio
                eliminamosOtroProceso();
                log_trace(logger, "Termino el case EXIT");
                sem_post(&sem_introducir_proceso_a_ready); 
                break;
            case SYSCALL_INIT_PROC:
                char * path = list_get(paqueteRespuesta, 3);
                int size = *(int*)list_get(paqueteRespuesta, 5);
                pthread_mutex_lock(&mutex_last_PID);
                int pidNuevo = ++last_PID;
                log_trace(logger, "Case init_proc con proceso (%d), path: %s, size: %d", pidNuevo, path, size);
                pthread_mutex_unlock(&mutex_last_PID);
                pthread_mutex_lock(&mutex_listasProcesos);
                nuevoProceso(pidNuevo, path, size, listasProcesos);
                pthread_mutex_unlock(&mutex_listasProcesos);
                aparecioOtroProceso();
                sem_post(&sem_introducir_proceso_a_ready);
                continuar_mismo_proceso = 1;
                break;
            case SYSCALL_IO:
                char * nombreIO = list_get(paqueteRespuesta, 3);
                int milisegundos = *(int*)list_get(paqueteRespuesta, 5);
                pthread_mutex_lock(&mutex_listasProcesos);
                cambiarEstado_EstadoActualConocido(proceso->PID, EXEC, BLOCKED, listasProcesos);
                proceso->ProcesadorQueLoEjecuta = NULL;
                pthread_mutex_unlock(&mutex_listasProcesos);

                pthread_t timerThread;
                Peticion * pet = crearPeticion(proceso->PID, milisegundos);
                pthread_create(&timerThread, NULL, temporizadorSuspenderThread, pet);
                pthread_detach(timerThread);

                encolarPeticionIO(nombreIO, pet); // Tambien hace señal a su semaforo

                actualizarEstimacion(proceso, alfa);
                log_info(logger, "## (%d) - Bloqueado por IO: %s", proceso->PID, nombreIO);
                break;
            case SYSCALL_DUMP_MEMORY:
                PIDySocket * infoDump;
                infoDump = malloc(sizeof(infoDump));
                infoDump->PID = proceso->PID;
                enviarSolicitudDumpMemory(proceso->PID, &(infoDump->socket));
                pthread_mutex_lock(&mutex_listasProcesos);
                cambiarEstado_EstadoActualConocido(proceso->PID, EXEC, BLOCKED, listasProcesos);
                proceso->ProcesadorQueLoEjecuta = NULL;
                pthread_mutex_unlock(&mutex_listasProcesos);
                actualizarEstimacion(proceso, alfa);
                pthread_t hiloConfirmacion;
                pthread_create(&hiloConfirmacion, NULL, confirmDumpMemoryThread, infoDump);
                pthread_detach(hiloConfirmacion);
                break;
            case INTERRUPT_ACKNOWLEDGE:
                log_trace(logger, "Case interrupt acknoledge");
                pthread_mutex_lock(&mutex_listasProcesos);
                cambiarEstado_EstadoActualConocido(proceso->PID, EXEC, READY, listasProcesos); 
                proceso->ProcesadorQueLoEjecuta = NULL;
                // CambiarEstado Ya actualiza a EXEC_ACT
                pthread_mutex_unlock(&mutex_listasProcesos);
                break;
            }
            log_trace(logger, "Termino el switch");
            eliminar_paquete_lista(paqueteRespuesta);
            log_trace(logger, "Termino un ciclo del do while");
        }while(continuar_mismo_proceso);
        log_trace(logger, "Termino de ejecutar los ciclos de do while, busco nuevo proceso a ejecutar");
    }
}

void * orderThread(void * _){
    char config_key_algoritmo[] = "ALGORITMO_CORTO_PLAZO";
    char * algoritmo = config_get_string_value(config, config_key_algoritmo);
    enum algoritmo algoritmo_enum = algoritmoStringToEnum(algoritmo);
    t_paquete * peticionInterrupt;
    while(1){
        sem_wait(&sem_ordenar_cola_ready);
        log_trace(logger, "Me pidieron que ordene la cola de ready");
        pthread_mutex_lock(&mutex_listasProcesos);
        log_trace(logger, "Ordenando cola de ready...");
        ordenar_cola_ready(listasProcesos, algoritmo_enum);
        t_PCB * procesoInterrumpido = procesoADesalojar(listasProcesos, algoritmo_enum);
        if(procesoInterrumpido != NULL){
            log_trace(logger, "Hay que desalojar a alquien...");
            peticionInterrupt = crear_paquete(PETICION_INTERRUPT_A_CPU);
            enviar_paquete(peticionInterrupt, procesoInterrumpido->ProcesadorQueLoEjecuta->SOCKET);
            log_trace(logger, "Ordenando cola de ready de nuevo...");
            ordenar_cola_ready(listasProcesos, algoritmo_enum);
            // Hay que reordenar para que el que se desalojo quede donde corresponde
        }
        pthread_mutex_unlock(&mutex_listasProcesos);
        log_trace(logger, "Listo, ordenamiento finalizado");
        sem_post(&sem_procesos_en_ready); // Esto se ejecuta cada vez que entra un nuevo proceso a ready, reordenandose asi la cola
        int a; 
        sem_getvalue(&sem_procesos_en_ready, &a);
        log_debug(logger, "Procesos en ready segun semaforo: %d", a);
        // Si no entra nada a ready (de NEW, BLOCKED o SUSP_READY) no se ejecuta
    }
}

void * ingresoAReadyThread(void * _){ // Planificador mediano y largo plazo
    int socketMemoria;
    int r;
    enum estado listaQueImporta;
    t_paquete * solicitud;
    t_list * respuesta;
    t_PCB * proceso;
    char config_key_algoritmo[] = "ALGORITMO_INGRESO_A_READY";
    enum algoritmo algoritmo = algoritmoStringToEnum(config_get_string_value(config, config_key_algoritmo));
    log_debug(logger, "Entrando a algoritmo introducir_proceso_a_ready");
    while(1){
        sem_wait(&sem_introducir_proceso_a_ready);
        log_debug(logger, "Inicio el proceso para pasar procesos a ready");
        pthread_mutex_lock(&mutex_listasProcesos);

        if (list_is_empty(listasProcesos[NEW]) && list_is_empty(listasProcesos[SUSP_READY])) {
            log_debug(logger, "Despertó el thread de ingresoAReady pero no hay procesos en NEW ni SUSP_READY. No hago nada.");
            pthread_mutex_unlock(&mutex_listasProcesos);
            continue;
        }

        if(list_is_empty(listasProcesos[SUSP_READY]))
            listaQueImporta = NEW; // Introduce nuevo proceso
        else
            listaQueImporta = SUSP_READY; // Dessuspende el proceso
        
        switch(algoritmo){
            case FIFO:
                proceso = list_get(listasProcesos[listaQueImporta], 0);
                break;
            case PMCP:
                proceso = list_get_minimum(listasProcesos[listaQueImporta], procesoMasCorto);
                break;
            default: 
                log_error(logger, "Error: Archivo de configuracion no detalla un algoritmo de introduccion de proceso a ready valido.");
                break;
            }
        
        log_debug(logger, "Proceso elegido para pasar a READY: (%d)", proceso->PID);
        pthread_mutex_unlock(&mutex_listasProcesos);
        {   //Enviar solicitud a memoria
            socketMemoria = conectarSocketClient(conexiones.ipYPuertoMemoria.IP, conexiones.ipYPuertoMemoria.puerto);
            handshakeMemoria(socketMemoria);
            if(listaQueImporta == NEW)
            {
            log_debug(logger, "Pido a memoria cargar un proceso de NEW");
            solicitud = crear_paquete(SOLICITUD_MEMORIA_NUEVO_PROCESO);
            agregar_a_paquete(solicitud, &(proceso->PID), sizeof(proceso->PID));
            agregar_a_paquete(solicitud, proceso->PATH, strlen(proceso->PATH)+1);
            agregar_a_paquete(solicitud, &(proceso->SIZE), sizeof(proceso->SIZE));
            }
            else
            {
            log_debug(logger, "Pido a memoria desuspender un proceso");
            solicitud = crear_paquete(SOLICITUD_MEMORIA_CARGA_SWAP);
            agregar_a_paquete(solicitud, &(proceso->PID), sizeof(proceso->PID));
            }
            log_debug(logger, "codOp = %d", solicitud->tipo_mensaje);
            enviar_paquete(solicitud, socketMemoria);
        }
        respuesta = recibir_paquete_lista(socketMemoria, MSG_WAITALL, &r);
        eliminar_paquete_lista(respuesta); // El contenido del paquete es vacio: Solo importa el codOp
        
        liberarConexion(socketMemoria);
        if(r == RESPUESTA_MEMORIA_PROCESO_CARGADO){
            log_debug(logger, "Hay espacio en memoria: Pasando (%d) a READY", proceso->PID);
            pthread_mutex_lock(&mutex_listasProcesos);
            if(listaQueImporta == NEW)
                cambiarEstado_EstadoActualConocido(proceso->PID, NEW, READY, listasProcesos);
            else
                cambiarEstado_EstadoActualConocido(proceso->PID, SUSP_READY, READY, listasProcesos);
            
            int hayProcesosPendientes = !list_is_empty(listasProcesos[NEW]) || !list_is_empty(listasProcesos[SUSP_READY]);
            
            pthread_mutex_unlock(&mutex_listasProcesos);
            log_trace(logger, "Se cambio el estado, ahora hay que ordenar la cola de ready");
            sem_post(&sem_ordenar_cola_ready);

            log_trace(logger, "Ya mande el mensaje de que ordene la cola, tengo que meter otro proceso?");
            log_trace(logger, "Validando si hay que meter otro proceso, list_size de: NEW=%d, SUSP_READY=%d",list_size(listasProcesos[NEW]), list_size(listasProcesos[SUSP_READY]));
            if(hayProcesosPendientes) // Si quedan procesos pruebo meter otro
                {
                log_trace(logger, "Si, tengo que meter otro proceso");
                sem_post(&sem_introducir_proceso_a_ready);
                }
            else{
                log_trace(logger, "No, no tengo que meter otro proceso");
            }
                                
        }
    }
}

void * IOThread(void * NOMBREYSOCKETIODTO)
{   
    IOThreadDTO this = *(IOThreadDTO*)NOMBREYSOCKETIODTO;
    t_paquete * paquete;
    Peticion * peticion;
    t_list * respuesta;
    while(1){
        {
            // Obtener peticion
            sem_wait(&this.datos->sem_peticiones);
            log_debug(logger, "Recibida peticion IO");
            pthread_mutex_lock(&this.datos->MUTEX_IO_SOCKETS);
            peticion = list_remove(this.datos->cola,0);
            pthread_mutex_unlock(&this.datos->MUTEX_IO_SOCKETS);
        }
        {
            //Emviar Peticion
            paquete = crear_paquete(PETICION_IO);
            agregar_a_paquete(paquete, &(peticion->PID), sizeof(peticion->PID));
            agregar_a_paquete(paquete, &(peticion->milisegundos), sizeof(peticion->milisegundos));
            enviar_paquete(paquete, *(this.SOCKET));
            eliminar_paquete(paquete);
        }
    
        // Recibir respuesta
        respuesta = recibir_paquete_lista(*(this.SOCKET), MSG_WAITALL, NULL);
        log_debug(logger, "IO me respondio");
        sem_wait(&(peticion->sem_estado));
        if(respuesta == NULL){ // Si se pierde la conexion, se termina el proceso
            log_error(logger, "Se perdio la conexion con IO: %s", this.datos->NOMBRE);
            peticion->estado = PETICION_FINALIZADA;
            cambiarEstado(peticion->PID, EXIT, listasProcesos);
            peticion->estado=PETICION_FINALIZADA;
            sem_post(&(peticion->sem_estado));
            liberarMemoria(peticion->PID);
        }else{                  // Si no se pierde la conexion, liberar el paquete y continuar a ready o susp_ready
        eliminar_paquete_lista(respuesta);


        if(peticion->estado == PETICION_BLOQUEADA){ // Si no se suspendio
            pthread_mutex_lock(&mutex_listasProcesos);
            cambiarEstado_EstadoActualConocido(peticion->PID, BLOCKED, READY, listasProcesos);
            pthread_mutex_unlock(&mutex_listasProcesos);
            log_info(logger, "## (%d) finalizo IO y pasa a READY", peticion->PID);
            peticion->estado = PETICION_FINALIZADA;
            sem_post(&(peticion->sem_estado));
            sem_post(&sem_ordenar_cola_ready);    
        }else if(peticion->estado == PETICION_SUSPENDIDA){ // Si se suspendio
            pthread_mutex_lock(&mutex_listasProcesos);
            cambiarEstado_EstadoActualConocido(peticion->PID, SUSP_BLOCKED, SUSP_READY, listasProcesos);
            pthread_mutex_unlock(&mutex_listasProcesos);
            log_info(logger, "## (%d) finalizo IO y pasa a SUSP_READY", peticion->PID);
            sem_post(&sem_introducir_proceso_a_ready);
            eliminarPeticion(peticion);
        }
        }

}
}

void * confirmDumpMemoryThread(void * Params){
    log_trace(logger, "inicio confirmDumpMemoryThread()");
    PIDySocket * infoDump = (PIDySocket*)Params;
    int resultado;
    t_list * paq = recibir_paquete_lista(infoDump->socket, MSG_WAITALL, &resultado);
    log_debug(logger, "Recibida respuesta dump memory");
    eliminar_paquete_lista(paq);
    if(resultado == RESPUESTA_DUMP_COMPLETADO){
        log_trace(logger, "Salio bien el dump");
        pthread_mutex_lock(&mutex_listasProcesos);
        cambiarEstado_EstadoActualConocido(infoDump->PID, BLOCKED, READY, listasProcesos);
        pthread_mutex_unlock(&mutex_listasProcesos);
    }else{
            log_trace(logger, "Salio mal el dump");
            pthread_mutex_lock(&mutex_listasProcesos);
            cambiarEstado_EstadoActualConocido(infoDump->PID, BLOCKED, EXIT, listasProcesos);
            pthread_mutex_unlock(&mutex_listasProcesos);
            liberarMemoria(infoDump->PID);
    }
    free(infoDump);
    liberarConexion(infoDump->socket);
    log_trace(logger, "Fin dump confirmDumpMemoryThread()");
    pthread_exit(NULL);
}

void post_sem_introducirAReady(){sem_post(&sem_introducir_proceso_a_ready);}


void * temporizadorSuspenderThread(void * param){
    Peticion * peticion = ((Peticion * )param);
    int tiempo = config_get_int_value(config, "TIEMPO_SUSPENSION");
    log_debug(logger, "Inicio de temporizador para suspender (%d) en %dms", peticion->PID, tiempo*1000);
    usleep(tiempo*1000); // microsegundos a milisegundos
    log_debug(logger, "Temporizador de (%d) finalizado", peticion->PID);

    sem_wait(&(peticion->sem_estado));

    if (peticion->estado == PETICION_BLOQUEADA)
    {
    pthread_mutex_lock(&mutex_listasProcesos);
        int r =cambiarEstado_EstadoActualConocido(peticion->PID, BLOCKED, SUSP_BLOCKED, listasProcesos);
    pthread_mutex_unlock(&mutex_listasProcesos);
    peticion->estado = PETICION_SUSPENDIDA;
    if (r!=0)
        log_debug(logger, "Cancelacion de la suspension de (%d), ya no esta mas bloqueado",peticion->PID);
    else
        log_debug(logger, "(%d) suspendido", peticion->PID);
    }
    sem_post(&(peticion->sem_estado));
    if(peticion->estado == PETICION_FINALIZADA)
    eliminarPeticion(peticion);
    pthread_exit(NULL);
}



