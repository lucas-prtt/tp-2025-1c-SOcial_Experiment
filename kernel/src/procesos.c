#include "procesos.h"


t_list * listasProcesos[7]; // Vector de lista para guardar procesos
// SEMAFOROS:
pthread_mutex_t mutex_listasProcesos;   // MUTEX para interactuar con listasProcesos[7]
sem_t sem_procesos_en_ready;            // Semeforo contador: Cantidad de procesos en READY
sem_t sem_ordenar_cola_ready;           // Representa peticion para ordenar la cola de ready y luego hacer signal a procesos en cola de ready (ya que se ejecuta al entrar un proceso nuevo)
sem_t sem_introducir_proceso_a_ready;   // Semaforo binario: Intenta mandar un proceso a memoria y a ready

///////////////

void procesos_c_inicializarSemaforos(){
    sem_init(&sem_procesos_en_ready, 0, 0);
    sem_init(&sem_ordenar_cola_ready, 0, 0);
    sem_init(&sem_introducir_proceso_a_ready, 0, 1);// Para esto se debe crear el proceso que envia de NEW a READY una vez que el proceso inicial esta en ready
}

void * dispatcherThread(void * IDYSOCKETDISPATCH){ // Maneja la mayor parte de la interaccion con las CPU a traves del socket dispatch

    IDySocket_CPU * cpu = (IDySocket_CPU*) IDYSOCKETDISPATCH;
    int codOp;
    t_list * paqueteRespuesta;
    t_PCB * proceso;
    while(1){
        {  // Extraer proceso de lista de READY, pasarlo a EXEC
            sem_wait(&sem_procesos_en_ready);
            pthread_mutex_lock(&mutex_listasProcesos);
            proceso = list_get(listasProcesos[READY], 0);
            cambiarEstado_EstadoActualConocido(proceso->PID, READY, EXEC, listasProcesos);
            pthread_mutex_unlock(&mutex_listasProcesos);
        }
        
        //TODO: ENVIAR PAQUETE DISPATCH

        paqueteRespuesta = recibir_paquete_lista(cpu->SOCKET, MSG_WAITALL, codOp);
        
        if (paqueteRespuesta = NULL){ // Si se cierra la conexion con el CPU, se cierra el hilo y se termina el proceso
            cambiarEstado_EstadoActualConocido(proceso->PID, EXEC, EXIT, listasProcesos);
            log_error(logger, "(%d) - Finaliza el proceso. Conexion con CPU (%d) perdida", proceso->PID, cpu->ID);
            pthread_exit(NULL);
        }
        
        //TODO: ACTUALIZAR PCB DEL KERNEL

        switch (codOp)//TODO: Cada caso con su logica: En funcion de codOp se hace cada syscall
        {
        case 0:
            break;
        }

        eliminar_paquete(paqueteRespuesta);
    }
}

void * orderThread(void * _){
    char config_key_algoritmo[] = "ALGORITMO_CORTO_PLAZO";
    char * algoritmo = config_get_string_value(config, config_key_algoritmo);
    enum algoritmo algoritmo_enum = algoritmoStringToEnum(algoritmo);
    while(1){
        wait(&sem_ordenar_cola_ready);
        pthread_mutex_lock(&mutex_listasProcesos);
        ordenar_cola_ready(listasProcesos, algoritmo_enum);
        pthread_mutex_unlock(&mutex_listasProcesos);
        sem_post(&sem_procesos_en_ready);
    }
}

void * newProcessThread(void * _){
    int socketMemoria;
    int respuesta;
    t_PCB * proceso;
    char config_key_algoritmo[] = "ALGORITMO_COLA_NEW";
    enum algoritmo algoritmo = algoritmoStringToEnum(config_get_string_value(config, config_key_algoritmo));
    while(1){
        sem_wait(&sem_introducir_proceso_a_ready);
        pthread_mutex_lock(&mutex_listasProcesos);
        switch(algoritmo){
        case FIFO:
            proceso = list_get(listasProcesos, 0);
            break;
        case PMCP:
            proceso = list_get_minimum(listasProcesos, procesoMasCorto);
            break;
        default: 
            // ERROR
            break;
        }
        pthread_mutex_unlock(&mutex_listasProcesos);
        socketMemoria = conectarSocketClient(conexiones.ipYPuertoMemoria.IP, conexiones.ipYPuertoMemoria.puerto);
        // TODO: Enviar peticion para entrar proceso
            // La peticion debe contener PID, PATH, TAMAÑO

        // TODO: Recibir respuesta
            // Sera un paquete con solo un int (enum?)
        respuesta = 1; // 1: Hay espacio y se introdujo, 0: No hay espacio y no se introdujo
        liberarConexion(socketMemoria);
        if(respuesta){
            pthread_mutex_lock(&mutex_listasProcesos);
            cambiarEstado_EstadoActualConocido(proceso->PID, NEW, READY, listasProcesos);
            pthread_mutex_unlock(&mutex_listasProcesos);
            sem_post(&sem_ordenar_cola_ready);
            if(!list_is_empty(listasProcesos[NEW])) // Si quedan procesos pruebo meter otro
                sem_post(&sem_introducir_proceso_a_ready);
        }
    }

}


