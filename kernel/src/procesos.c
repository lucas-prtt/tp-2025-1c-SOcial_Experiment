#include "procesos.h"


// VARIABLES GLOBALES: 
t_list * listasProcesos[7];      // Vector de lista para guardar procesos
t_list * lista_peticionesIO;     // Lista donde se guarda por cada IO su nombre, cola de peticiones y un Semaforo. La cola se maneja por FIFO
int last_PID = 0;                // Estaba en 1
int qProcesosMolestando = 0;     // Cantidad de procesos que quedan ejecutar
///////////////////////////////////

// SEMAFOROS:
pthread_mutex_t mutex_listasProcesos    // MUTEX para interactuar con listasProcesos[7]
 = PTHREAD_MUTEX_INITIALIZER        ;   // 
pthread_mutex_t mutex_peticionesIO      // MUTEX para acceder a lista_peticionesIO
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
    lista_peticionesIO = list_create();
    
}
void * dispatcherThread(void * IDYSOCKETDISPATCH){ // Maneja la mayor parte de la interaccion con las CPU a traves del socket dispatch
    IDySocket_CPU * cpuDispatch = (IDySocket_CPU*) IDYSOCKETDISPATCH;
    IDySocket_CPU * cpuInterrupt = buscarCPUInterruptPorID(cpuDispatch->ID);
    int codOp;
    t_list * paqueteRespuesta;
    t_paquete * paqueteEnviado;
    t_PCB * proceso;
    int * pid;
    int continuar_mismo_proceso;
    float alfa = atof(config_get_string_value(config, "ALFA"));

    while(1) {
        {  // Extraer proceso de lista de READY, pasarlo a EXEC
            int a; 
            sem_getvalue(&sem_procesos_en_ready, &a);
            log_trace(logger, "Procesos en ready segun semaforo: %d", a);
            sem_wait(&sem_procesos_en_ready);
            log_trace(logger, "Extrayendo proceso de la cola de ready.");
            pthread_mutex_lock(&mutex_listasProcesos);
            proceso = list_get(listasProcesos[READY], 0);
            cambiarEstado_EstadoActualConocido(proceso->PID, READY, EXEC, listasProcesos);
            proceso->ProcesadorQueLoEjecutaDispatch = cpuDispatch;
            proceso->ProcesadorQueLoEjecutaInterrupt = cpuInterrupt;
            pthread_mutex_unlock(&mutex_listasProcesos);
            log_debug(logger, "Se eligio el proceso (%d) para ejecutar", proceso->PID);
        }
        do{
            
            continuar_mismo_proceso = 0;
            paqueteEnviado = crear_paquete(ASIGNACION_PROCESO_CPU);
            agregar_a_paquete(paqueteEnviado, &(proceso->PID), sizeof(proceso->PID));
            agregar_a_paquete(paqueteEnviado, &(proceso->PC), sizeof(proceso->PC));
            enviar_paquete(paqueteEnviado, cpuDispatch->SOCKET);
            eliminar_paquete(paqueteEnviado);
            log_trace(logger, "Se asigno el proceso (%d) a la cpu %d en PC %d", proceso->PID, cpuDispatch->ID, proceso->PC);
            log_trace(logger, "El hilo de CPU %d se bloquea esperando respuesta", cpuDispatch->ID);
            
            paqueteRespuesta = recibir_paquete_lista(cpuDispatch->SOCKET, MSG_WAITALL, &codOp);
            log_trace(logger, "Se recibio un paquete de respuesta de proceso (%d) de la cpu %d", proceso->PID, cpuDispatch->ID);
            
            if (paqueteRespuesta == NULL){ // Si se cierra la conexion con el CPU, se cierra el hilo y se termina el proceso
                pthread_mutex_lock(&mutex_listasProcesos);
                cambiarEstado_EstadoActualConocido(proceso->PID, EXEC, EXIT, listasProcesos);
                eliminamosOtroProceso();
                proceso->ProcesadorQueLoEjecutaDispatch = NULL;
                proceso->ProcesadorQueLoEjecutaInterrupt = NULL;

                pthread_mutex_unlock(&mutex_listasProcesos);
                liberarMemoria(proceso->PID);
                log_warning(logger, "(%d) - Finaliza el proceso. Conexion con CPU (%d) perdida", proceso->PID, cpuDispatch->ID);
                pthread_exit(NULL);
            }
            int nuevoPC = *(int*)list_get(paqueteRespuesta, 1);
            log_trace(logger, "El parquete dice: Codop: %d,  proceso (%d) de la cpu %d PC paso a %d",codOp ,  proceso->PID, cpuDispatch->ID, *(int*)list_get(paqueteRespuesta, 1));
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
                pid = list_get(paqueteRespuesta, 3);
                if(*pid != proceso->PID){
                    log_error(logger, "Se recibio un EXIT de otro proceso (%d en vez de %d)", *pid, proceso->PID);
                }
                pthread_mutex_lock(&mutex_listasProcesos);
                cambiarEstado_EstadoActualConocido(proceso->PID, EXEC, EXIT, listasProcesos);
                log_trace(logger, "Se va el proceso (%d) a exit", proceso->PID);
                proceso->ProcesadorQueLoEjecutaDispatch = NULL;
                proceso->ProcesadorQueLoEjecutaInterrupt = NULL;

                pthread_mutex_unlock(&mutex_listasProcesos);
                liberarMemoria(proceso->PID); // Envia mensaje a Memoria para liberar el espacio
                eliminamosOtroProceso();
                sem_post(&evaluarFinKernel);
                sem_post(&sem_introducir_proceso_a_ready); 
                log_trace(logger, "Termino el case EXIT");
                break;
            case SYSCALL_INIT_PROC:
                char * path = list_get(paqueteRespuesta, 3);
                int size = *(int*)list_get(paqueteRespuesta, 5);
                pid = list_get(paqueteRespuesta, 7);
                if(*pid != proceso->PID){
                    log_error(logger, "Se recibio un INIT_PROC de otro proceso (%d en vez de %d)", *pid, proceso->PID);
                }
                pthread_mutex_lock(&mutex_last_PID);
                int pidNuevo = ++last_PID;
                log_trace(logger, "Case init_proc con proceso (%d), path: %s, size: %d", pidNuevo, path, size);
                pthread_mutex_unlock(&mutex_last_PID);
                pthread_mutex_lock(&mutex_listasProcesos);
                nuevoProceso(pidNuevo, path, size, listasProcesos);
                pthread_mutex_unlock(&mutex_listasProcesos);
                sem_post(&sem_introducir_proceso_a_ready);
                continuar_mismo_proceso = 1;
                break;
            case SYSCALL_IO:
                char * nombreIO = list_get(paqueteRespuesta, 3);
                int milisegundos = *(int*)list_get(paqueteRespuesta, 5);
                pid = list_get(paqueteRespuesta, 7);
                if(*pid != proceso->PID){
                    log_error(logger, "Se recibio un IO de otro proceso (%d en vez de %d)", *pid, proceso->PID);
                }


                Peticion * pet = crearPeticion(proceso->PID, milisegundos);
                pthread_mutex_lock(&mutex_peticionesIO);
                int instanciasDeLaIo = encolarPeticionIO(nombreIO, pet, lista_peticionesIO); // Tambien hace señal a su semaforo
                pthread_mutex_unlock(&mutex_peticionesIO);
                proceso->ProcesadorQueLoEjecutaDispatch = NULL;
                proceso->ProcesadorQueLoEjecutaInterrupt = NULL;
                pthread_mutex_lock(&mutex_listasProcesos);
                if(instanciasDeLaIo > 0){
                    pthread_t timerThread;
                    pthread_create(&timerThread, NULL, temporizadorSuspenderThread, pet);
                    pthread_detach(timerThread);
                    cambiarEstado_EstadoActualConocido(proceso->PID, EXEC, BLOCKED, listasProcesos);
                    actualizarEstimacion(proceso, alfa);
                    log_info(logger, "## (%d) - Bloqueado por IO: %s", proceso->PID, nombreIO);
                    }
                else{
                    log_warning(logger, "(%d) - Pasa a EXIT por IO invalida", proceso->PID);
                    eliminarPeticion(pet);
                    cambiarEstado_EstadoActualConocido(proceso->PID, EXEC, EXIT, listasProcesos);
                    eliminamosOtroProceso();
                    liberarMemoria(proceso->PID); // Envia mensaje a Memoria para liberar el espacio
                    sem_post(&evaluarFinKernel);
                    sem_post(&sem_introducir_proceso_a_ready); 
                }
                pthread_mutex_unlock(&mutex_listasProcesos);
        

                break;
            case SYSCALL_DUMP_MEMORY:
                PIDySocket * infoDump;
                infoDump = malloc(sizeof(PIDySocket));
                infoDump->PID = proceso->PID;
                enviarSolicitudDumpMemory(proceso->PID, &(infoDump->socket));
                pthread_mutex_lock(&mutex_listasProcesos);
                cambiarEstado_EstadoActualConocido(proceso->PID, EXEC, BLOCKED, listasProcesos);
                proceso->ProcesadorQueLoEjecutaDispatch = NULL;
                proceso->ProcesadorQueLoEjecutaInterrupt = NULL;
                pthread_mutex_unlock(&mutex_listasProcesos);
                actualizarEstimacion(proceso, alfa);
                pthread_t hiloConfirmacion;
                pthread_create(&hiloConfirmacion, NULL, confirmDumpMemoryThread, infoDump);
                pthread_detach(hiloConfirmacion);
                break;
            case INTERRUPT_ACKNOWLEDGE:
                log_trace(logger, "Case interrupt acknoledge");
                pid = list_get(paqueteRespuesta, 3);
                if(*pid != proceso->PID){
                    log_error(logger, "Se recibio un interrupt acknowledge de otro proceso (%d en vez de %d)", *pid, proceso->PID);
                }else{
                pthread_mutex_lock(&mutex_listasProcesos);
                cambiarEstado_EstadoActualConocido(proceso->PID, EXEC, READY, listasProcesos); 
                proceso->ProcesadorQueLoEjecutaDispatch = NULL;
                proceso->ProcesadorQueLoEjecutaInterrupt = NULL;
		sem_post(&sem_procesos_en_ready);
                // CambiarEstado Ya actualiza a EXEC_ACT
                pthread_mutex_unlock(&mutex_listasProcesos);
                }
                break;
            default:
                log_error(logger, "Se recibio una respuesta de CPU no valida: codop = %d", codOp);
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
            agregar_a_paquete(peticionInterrupt, &(procesoInterrumpido->PID), sizeof(int));
            enviar_paquete(peticionInterrupt, procesoInterrumpido->ProcesadorQueLoEjecutaInterrupt->SOCKET);
            eliminar_paquete(peticionInterrupt);
            log_debug(logger, "Peticion de desalojo enviada: Se debe interrumpir (%d)", procesoInterrumpido->PID);
            log_trace(logger, "Ordenando cola de ready de nuevo...");
            ordenar_cola_ready(listasProcesos, algoritmo_enum);
            // Hay que reordenar para que el que se desalojo quede donde corresponde
        }
        pthread_mutex_unlock(&mutex_listasProcesos);
        log_trace(logger, "Listo, ordenamiento finalizado");
        sem_post(&sem_procesos_en_ready); // Esto se ejecuta cada vez que entra un nuevo proceso a ready, reordenandose asi la cola
        int a; 
        sem_getvalue(&sem_procesos_en_ready, &a);
        log_trace(logger, "Procesos en ready segun semaforo: %d", a);
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
        log_trace(logger, "Inicio el proceso para pasar procesos a ready");
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
        
        log_trace(logger, "Proceso elegido para pasar a READY: (%d)", proceso->PID);
        pthread_mutex_unlock(&mutex_listasProcesos);
        {   //Enviar solicitud a memoria
            socketMemoria = conectarSocketClient(conexiones.ipYPuertoMemoria.IP, conexiones.ipYPuertoMemoria.puerto);
            handshakeMemoria(socketMemoria);
            if(listaQueImporta == NEW)
            {
            log_trace(logger, "Pido a memoria cargar un proceso de NEW");
            solicitud = crear_paquete(SOLICITUD_MEMORIA_NUEVO_PROCESO);
            agregar_a_paquete(solicitud, &(proceso->PID), sizeof(proceso->PID));
            agregar_a_paquete(solicitud, proceso->PATH, strlen(proceso->PATH)+1);
            agregar_a_paquete(solicitud, &(proceso->SIZE), sizeof(proceso->SIZE));
            }
            else
            {
            log_trace(logger, "Pido a memoria desuspender un proceso");
            solicitud = crear_paquete(SOLICITUD_MEMORIA_CARGA_SWAP);
            agregar_a_paquete(solicitud, &(proceso->PID), sizeof(proceso->PID));
            }
            log_trace(logger, "codOp = %d", solicitud->tipo_mensaje);
            enviar_paquete(solicitud, socketMemoria);
            eliminar_paquete(solicitud);
        }
        respuesta = recibir_paquete_lista(socketMemoria, MSG_WAITALL, &r);
        eliminar_paquete_lista(respuesta); // El contenido del paquete es vacio: Solo importa el codOp
        
        liberarConexion(socketMemoria);
        if(r == RESPUESTA_MEMORIA_PROCESO_CARGADO){
            log_trace(logger, "Hay espacio en memoria: Pasando (%d) a READY", proceso->PID);
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

void * IOThread(void * NOMBREYSOCKETIO)
{   
    log_debug(logger, "Se creo el IOTHREAD");
    t_paquete * paquete;
    Peticion * peticion;
    PeticionesIO * peticiones = NULL;
    t_list * respuesta;
    NombreySocket_IO * io = (NombreySocket_IO*)NOMBREYSOCKETIO;
    pthread_mutex_lock(&mutex_peticionesIO);
    for(int i = 0; i < list_size( lista_peticionesIO); i++){
        if(!strcmp(((PeticionesIO*)list_get(lista_peticionesIO, i))->nombre, io->NOMBRE))
        {
            peticiones = (PeticionesIO*)list_get(lista_peticionesIO, i);
            break;
        }
    }
    log_debug(logger, "Se le encontro cola de peticiones al IO");
    int valido = 0;
    char caracterInutil;
    if(peticiones == NULL){
    peticiones = malloc(sizeof(PeticionesIO));
    peticiones->nombre = io->NOMBRE;
    peticiones->instancias = 1;
    pthread_mutex_init(&(peticiones->MUTEX_cola), NULL);
    sem_init(&(peticiones->sem_peticiones), 0, 0);
    peticiones->cola = list_create();
    list_add(lista_peticionesIO, peticiones);
    log_debug(logger, "Se le asigno cola de peticiones al IO");
    }else{
        peticiones->instancias++;
    }
    pthread_mutex_unlock(&mutex_peticionesIO);
    while(1){
        {
            // Obtener peticion
            sem_wait(&peticiones->sem_peticiones);

            valido = recv(io->SOCKET,&caracterInutil, 1, MSG_PEEK | MSG_DONTWAIT);
            int cerrar = !valido && errno != EAGAIN && errno != EWOULDBLOCK;
            if (cerrar){
                log_debug(logger, "Socket de IO cerrado. No se enviara nada");
                pthread_mutex_lock(&mutex_peticionesIO);
                peticiones->instancias--;
                pthread_mutex_unlock(&mutex_peticionesIO);
                sem_post(&(peticiones->sem_peticiones));
                close(io->SOCKET);
            }
            log_debug(logger, "Recibida peticion IO");

            pthread_mutex_lock(&mutex_peticionesIO);
            if(peticiones->instancias <=0){ // Se pierde conexion y no hay mas instancias
                pthread_mutex_unlock(&mutex_peticionesIO);
                pthread_mutex_lock(&mutex_listasProcesos);
                list_iterate(peticiones->cola, terminarProcesoPorPeticionInvalida); // Destruye peticiones o las pone como "Finalizadas" para que el temporizador las ignore
                pthread_mutex_unlock(&mutex_listasProcesos);
                sem_post(&evaluarFinKernel);
                sem_post(&sem_introducir_proceso_a_ready); 
                return NULL;
            }else{
                pthread_mutex_unlock(&mutex_peticionesIO);
                if(cerrar){// Se pierde conexion y hay mas instancias
                    return NULL;
                }
                //Else, no se pierde conexion
            }

            pthread_mutex_lock(&(peticiones->MUTEX_cola));
            peticion = list_remove(peticiones->cola,0);
            pthread_mutex_unlock(&(peticiones->MUTEX_cola));
        }
        {
            //Emviar Peticion
            paquete = crear_paquete(PETICION_IO);
            agregar_a_paquete(paquete, &(peticion->PID), sizeof(peticion->PID));
            agregar_a_paquete(paquete, &(peticion->milisegundos), sizeof(peticion->milisegundos));
            enviar_paquete(paquete, io->SOCKET);
            eliminar_paquete(paquete);
        }
    
        // Recibir respuesta
        respuesta = recibir_paquete_lista(io->SOCKET, MSG_WAITALL, NULL);
        log_debug(logger, "IO me respondio");
        sem_wait(&(peticion->sem_estado));
        if(respuesta == NULL){ // Si se pierde la conexion, se termina el proceso
            log_warning(logger, "Se perdio la conexion con IO %s durante la ejecucion de una entrada/salida. Se envia el proceso (%d) a EXIT", io->NOMBRE, peticion->PID);
            peticiones->instancias--;
            close(io->SOCKET);
            pthread_mutex_lock(&mutex_listasProcesos);
            cambiarEstado(peticion->PID, EXIT, listasProcesos);
            liberarMemoria(peticion->PID);
            eliminamosOtroProceso();
            pthread_mutex_unlock(&mutex_listasProcesos);
            pthread_mutex_lock(&mutex_peticionesIO);
            if(peticiones->instancias <=0){
                pthread_mutex_unlock(&mutex_peticionesIO);
                pthread_mutex_lock(&mutex_listasProcesos);
                list_iterate(peticiones->cola, terminarProcesoPorPeticionInvalida);
                pthread_mutex_unlock(&mutex_listasProcesos);    
                sem_post(&evaluarFinKernel);
                sem_post(&sem_introducir_proceso_a_ready); 
            }else{
            pthread_mutex_unlock(&mutex_peticionesIO);
            }
            if (peticion->estado == PETICION_BLOQUEADA)
            peticion->estado = PETICION_FINALIZADA;
            else{
                eliminarPeticion(peticion);
            }
            return NULL;
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
    if(resultado == RESPUESTA_DUMP_COMPLETADO){
        log_trace(logger, "Salio bien el dump");
        pthread_mutex_lock(&mutex_listasProcesos);
        cambiarEstado_EstadoActualConocido(infoDump->PID, BLOCKED, READY, listasProcesos);
        pthread_mutex_unlock(&mutex_listasProcesos);
        sem_post(&sem_ordenar_cola_ready);
    }else{
            log_error(logger, "Salio mal el dump");
            if(resultado == -42 && paq == NULL){
                log_error(logger, "Se perdio la conexion con Memoria");
            }else if(resultado == RESPUESTA_DUMP_ERROR){
                log_error(logger, "Memoria falló al completar el dump");
            }else{
                log_error(logger, "Se produjo un error no previsto");
            }
            pthread_mutex_lock(&mutex_listasProcesos);
            cambiarEstado_EstadoActualConocido(infoDump->PID, BLOCKED, EXIT, listasProcesos);
            pthread_mutex_unlock(&mutex_listasProcesos);
            liberarMemoria(infoDump->PID);
    }
    eliminar_paquete_lista(paq);
    liberarConexion(infoDump->socket);
    free(infoDump);
    log_trace(logger, "Fin dump confirmDumpMemoryThread()");
    pthread_exit(NULL);
}

void post_sem_introducirAReady(){sem_post(&sem_introducir_proceso_a_ready);}


void * temporizadorSuspenderThread(void * param){
    Peticion * peticion = ((Peticion * )param);
    int tiempo = config_get_int_value(config, "TIEMPO_SUSPENSION");
    log_trace(logger, "Inicio de temporizador para suspender (%d) en %dms", peticion->PID, tiempo*1000);
    usleep(tiempo*1000); // microsegundos a milisegundos
    log_trace(logger, "Temporizador de (%d) finalizado", peticion->PID);

    sem_wait(&(peticion->sem_estado));
    if(peticion->estado == PETICION_FINALIZADA)
        eliminarPeticion(peticion);
    else
    if (peticion->estado == PETICION_BLOQUEADA)
    {
    pthread_mutex_lock(&mutex_listasProcesos);
        int r =cambiarEstado_EstadoActualConocido(peticion->PID, BLOCKED, SUSP_BLOCKED, listasProcesos);
    pthread_mutex_unlock(&mutex_listasProcesos);
    peticion->estado = PETICION_SUSPENDIDA;
    if (r!=0)
        log_trace(logger, "Cancelacion de la suspension de (%d), ya no esta mas bloqueado",peticion->PID);
    else{
        enviarSolicitudSuspensionProceso(peticion->PID);
        log_debug(logger, "(%d) suspendido", peticion->PID);
        }
    sem_post(&(peticion->sem_estado));
    }
    
    sem_post(&sem_introducir_proceso_a_ready);
    pthread_exit(NULL);
}



