#include "procesos.h"


t_list * listasProcesos[7]; // Vector de lista para guardar procesos
// SEMAFOROS:
pthread_mutex_t mutex_listasProcesos;   // MUTEX para interactuar con listasProcesos[7]
sem_t sem_procesos_en_ready;            // Semeforo contador: Cantidad de procesos en READY
///////////////

void procesos_c_inicializarSemaforos(){
    sem_init(&sem_procesos_en_ready, 0, 0);
}

void * dispatcher(void * IDYSOCKETDISPATCH){ // Maneja la mayor parte de la interaccion con las CPU a traves del socket dispatch

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

