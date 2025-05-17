#include "procesos.h"


// VARIABLES GLOBALES: 
t_list * listasProcesos[7];      // Vector de lista para guardar procesos
t_list * lista_peticionesIO;     // Lista donde se guarda por cada IO su nombre, cola de peticiones y un Semaforo. La cola se maneja por FIFO
int last_PID = 1;                //
///////////////////////////////////

// SEMAFOROS:
pthread_mutex_t mutex_listasProcesos;   // MUTEX para interactuar con listasProcesos[7]
pthread_mutex_t mutex_peticionesIO;     // MUTEX para acceder a lista_peticionesIO
pthread_mutex_t mutex_last_PID;         // MUTEX para acceder a last_PID
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

    IDySocket_CPU * cpu = (IDySocket_CPU*) IDYSOCKETDISPATCH;
    int codOp;
    t_list * paqueteRespuesta;
    t_paquete * paqueteEnviado;
    t_PCB * proceso;
    int continuar_mismo_proceso;
    while(1){
        {  // Extraer proceso de lista de READY, pasarlo a EXEC
            sem_wait(&sem_procesos_en_ready);
            pthread_mutex_lock(&mutex_listasProcesos);
            proceso = list_get(listasProcesos[READY], 0);
            cambiarEstado_EstadoActualConocido(proceso->PID, READY, EXEC, listasProcesos);
            pthread_mutex_unlock(&mutex_listasProcesos);
        }
        continuar_mismo_proceso = 0;
        do{
            
            paqueteEnviado = crear_paquete(ASIGNACION_PROCESO_CPU);
            agregar_a_paquete(paqueteEnviado, &(proceso->PID), sizeof(proceso->PID));
            agregar_a_paquete(paqueteEnviado, &(proceso->PC), sizeof(proceso->PC));
            enviar_paquete(paqueteEnviado, cpu->SOCKET);
            eliminar_paquete(paqueteEnviado);
            paqueteRespuesta = recibir_paquete_lista(cpu->SOCKET, MSG_WAITALL, &codOp);

            if (paqueteRespuesta == NULL){ // Si se cierra la conexion con el CPU, se cierra el hilo y se termina el proceso
                pthread_mutex_lock(&mutex_listasProcesos);
                cambiarEstado_EstadoActualConocido(proceso->PID, EXEC, EXIT, listasProcesos);
                pthread_mutex_unlock(&mutex_listasProcesos);
                log_error(logger, "(%d) - Finaliza el proceso. Conexion con CPU (%d) perdida", proceso->PID, cpu->ID);
                pthread_exit(NULL);
            }
            
            int deltaPC = *(int*)list_get(paqueteRespuesta, 1);
            proceso->PC += deltaPC;

            // Las metricas (MT y ME) se actualizan solas en cambiarDeEstado()
            // cambiarDeEstado() tambien maneja los inicios y finalizaciones de los "timers" para cada estado

            switch (codOp)//TODO: Cada caso con su logica: En funcion de codOp se hace cada syscall
            {
            case SYSCALL_EXIT:
                pthread_mutex_lock(&mutex_listasProcesos);
                cambiarEstado_EstadoActualConocido(proceso->PID, EXEC, EXIT, listasProcesos);
                pthread_mutex_unlock(&mutex_listasProcesos);
                // TODO: pedirle a memoria que libere el espacio
                log_info(logger, "(%d) - Finaliza el proceso", proceso->PID);
                sem_post(&sem_introducir_proceso_a_ready); 
                break;
            case SYSCALL_INIT_PROC:
                char * path = list_get(paqueteRespuesta, 3);
                int size = *(int*)list_get(paqueteRespuesta, 5);
                pthread_mutex_lock(&mutex_last_PID);
                int pidNuevo = ++last_PID;
                pthread_mutex_unlock(&mutex_last_PID);
                pthread_mutex_lock(&mutex_listasProcesos);
                nuevoProceso(pidNuevo, path, size, listasProcesos);
                pthread_mutex_unlock(&mutex_listasProcesos);
                log_info(logger, "(%d) Se crea el proceso - Estado: NEW", pidNuevo);
                sem_post(&sem_introducir_proceso_a_ready);
                continuar_mismo_proceso = 1;
                break;
            case SYSCALL_IO:
                char * nombreIO = list_get(paqueteRespuesta, 3);
                int milisegundos = *(int*)list_get(paqueteRespuesta, 5);
                pthread_mutex_lock(&mutex_listasProcesos);
                cambiarEstado_EstadoActualConocido(proceso->PID, EXEC, BLOCKED, listasProcesos);
                pthread_mutex_unlock(&mutex_listasProcesos);
                // TODO: CHECKPOINT 3: TEMPORIZADOR
                pthread_mutex_lock(&mutex_peticionesIO);
                encolarPeticionIO(proceso->PID, nombreIO, milisegundos, lista_peticionesIO); // Tambien hace señal a su semaforo
                pthread_mutex_unlock(&mutex_peticionesIO);
                log_info(logger, "(%d) - Bloqueado por IO: %s", proceso->PID, nombreIO);
                break;
            }
            eliminar_paquete_lista(paqueteRespuesta);
        }while(continuar_mismo_proceso);
    }
}

void * orderThread(void * _){
    char config_key_algoritmo[] = "ALGORITMO_CORTO_PLAZO";
    char * algoritmo = config_get_string_value(config, config_key_algoritmo);
    enum algoritmo algoritmo_enum = algoritmoStringToEnum(algoritmo);
    while(1){
        sem_wait(&sem_ordenar_cola_ready);
        pthread_mutex_lock(&mutex_listasProcesos);
        ordenar_cola_ready(listasProcesos, algoritmo_enum);
        pthread_mutex_unlock(&mutex_listasProcesos);
        sem_post(&sem_procesos_en_ready);
    }
}

void * ingresoAReadyThread(void * _){ // Planificador mediano y largo plazo
    int socketMemoria;
    int respuesta;
    enum estado listaQueImporta;
    t_PCB * proceso;
    char config_key_algoritmo[] = "ALGORITMO_INGRESO_A_READY";
    enum algoritmo algoritmo = algoritmoStringToEnum(config_get_string_value(config, config_key_algoritmo));
    while(1){
        sem_wait(&sem_introducir_proceso_a_ready);
        pthread_mutex_lock(&mutex_listasProcesos);
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
        pthread_mutex_unlock(&mutex_listasProcesos);
        socketMemoria = conectarSocketClient(conexiones.ipYPuertoMemoria.IP, conexiones.ipYPuertoMemoria.puerto);
        // TODO: Enviar peticion para entrar proceso
            // La peticion debe contener PID, PATH, TAMAÑO
        // TODO: Enviar peticion para mover de SWAP a Memoria

        // TODO: Recibir respuesta
            // Sera un paquete con solo un int (enum?)
        respuesta = 1; // 1: Hay espacio y se introdujo, 0: No hay espacio y no se introdujo
        liberarConexion(socketMemoria);
        if(respuesta){
            pthread_mutex_lock(&mutex_listasProcesos);
            if(listaQueImporta == NEW)
                cambiarEstado_EstadoActualConocido(proceso->PID, NEW, READY, listasProcesos);
            else
                cambiarEstado_EstadoActualConocido(proceso->PID, SUSP_READY, READY, listasProcesos);
            pthread_mutex_unlock(&mutex_listasProcesos);
            sem_post(&sem_ordenar_cola_ready);
            if(!list_is_empty(listasProcesos[NEW]) || !list_is_empty(listasProcesos[SUSP_READY])) // Si quedan procesos pruebo meter otro
                sem_post(&sem_introducir_proceso_a_ready);
        }
    }

}

void * IOThread(void * NOMBREYSOCKETIO)
{   
    t_paquete * paquete;
    Peticion * peticion;
    t_list * respuesta;
    NombreySocket_IO * io = (NombreySocket_IO*)NOMBREYSOCKETIO;
    PeticionesIO * peticiones = malloc(sizeof(PeticionesIO));
    peticiones->nombre = malloc(strlen(io->NOMBRE)+1);
    memcpy(peticiones->nombre, io->NOMBRE, strlen(io->NOMBRE)+1);
    sem_init(&(peticiones->sem_peticiones), 0, 0);
    peticiones->cola = list_create();
    pthread_mutex_lock(&mutex_peticionesIO);
    list_add(lista_peticionesIO, peticiones);
    pthread_mutex_unlock(&mutex_peticionesIO);
    while(1){
        {
            // Obtener peticion
            sem_wait(&peticiones->sem_peticiones);
            pthread_mutex_lock(&mutex_peticionesIO);
            peticion = list_remove(peticiones->cola,0);
            pthread_mutex_unlock(&mutex_peticionesIO);
        }
        {
            //Emviar Peticion
            paquete = crear_paquete(PETICION_IO);
            agregar_a_paquete(paquete, &(peticion->PID), sizeof(peticion->PID));
            agregar_a_paquete(paquete, &(peticion->milisegundos), sizeof(peticion->milisegundos));
            enviar_paquete(paquete, io->SOCKET);
            eliminar_paquete(paquete);
        }
        {
            // Recibir peticion
            respuesta = recibir_paquete_lista(io->SOCKET, MSG_WAITALL, NULL);
            if(respuesta == NULL){
                log_error(logger, "Se perdio la conexion con IO: %s", io->NOMBRE);
                // TODO: EXIT al proceso actual y todos los procesos con peticiones
                free(peticion);
                pthread_exit(NULL);
            }
            eliminar_paquete_lista(respuesta);
            pthread_mutex_lock(&mutex_listasProcesos);
            cambiarEstado_EstadoActualConocido(peticion->PID, BLOCKED, READY, listasProcesos);
            pthread_mutex_unlock(&mutex_listasProcesos);
        }
        // TODO: Actualizar metricas de estado
        free(peticion);
        sem_post(&sem_ordenar_cola_ready);    
        // Cuando se suspendan, hay que ver si esta suspendido o no para mandar señal a un proceso de desuspender
        // Cuando se implemente temporizador de suspension, eliminar el temporizador
    }

}