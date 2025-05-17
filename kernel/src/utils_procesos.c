#include "utils_procesos.h"





bool procesoEnExit(t_PCB * proceso, t_list * listasProcesos[]){
    #ifndef __INTELLISENSE__ 
    bool punterosIguales(void *puntero){
        return proceso == puntero;
    }
    return list_any_satisfy(listasProcesos[EXIT], punterosIguales);
    #endif

}
void loguearMetricasDeEstado_PorPCB(t_PCB * proceso, t_list * listasProcesos[]){
    int tiempoExit = proceso->MT[EXIT];
    if(procesoEnExit(proceso, listasProcesos)){
    tiempoExit += milisegundosDesde(proceso->tiempoEnEstado.inicio);
    }
    log_debug(logger, 
        "(%d) - Métricas de estado: "
        "NEW (%d) (%d), "
        "READY (%d) (%d), "
        "EXEC (%d) (%d), "
        "EXIT (%d) (%d), "
        "BLOCKED (%d) (%d), "
        "SUSP_BLOCKED (%d) (%d), "
        "SUSP_READY (%d) (%d)",
        proceso->PID,
        proceso->ME[NEW], proceso->MT[NEW],
        proceso->ME[READY], proceso->MT[READY],
        proceso->ME[EXEC], proceso->MT[EXEC],
        proceso->ME[EXIT], tiempoExit,
        proceso->ME[BLOCKED], proceso->MT[BLOCKED],
        proceso->ME[SUSP_BLOCKED], proceso->MT[SUSP_BLOCKED],
        proceso->ME[SUSP_READY], proceso->MT[SUSP_READY]
    );
}





t_PCB * crearPCB(int id, char * path, int size){
    t_PCB * pcb = malloc(sizeof(pcb));
    pcb->PID = id;
    pcb->SIZE = size;
    pcb->PATH = path;
    pcb->EJC_ANT = 0;
    pcb->EJC_ACT = 0;
    pcb->EST = 0;
    pcb->PC = 0;
    for (int i = 0; i<7; i++){
    pcb->ME[i] = 0;
    pcb->MT[i] = 0;
    }
    return pcb;
}

void nuevoProceso(int id, char * path, int size, t_list * listaProcesos[]){
    t_PCB * nuevoProceso = crearPCB(id, path, size);
    list_add(listaProcesos[NEW], nuevoProceso);
    nuevoProceso->ME[NEW]++;
    timeDifferenceStart(&(nuevoProceso->tiempoEnEstado));
    log_info(logger, "## (%d) Se crea el proceso - Estado:NEW", id);
}

void cambiarEstado(int idProceso, enum estado estadoSiguiente, t_list * listaProcesos[]){
    log_debug(logger, "Se pide cambiar PID:%d a %s", idProceso, estadoAsString(estadoSiguiente));
    enum estado estadoActual;
    for(int i = 0; i<7; i++){
        if(encontrarProcesoPorPIDYLista(listaProcesos[i], idProceso) != NULL){
            estadoActual = i;
            break;
            }
    }
    cambiarEstado_EstadoActualConocido(idProceso, estadoActual, estadoSiguiente, listaProcesos);
}

void cambiarEstado_EstadoActualConocido(int idProceso, enum estado estadoActual, enum estado estadoSiguiente, t_list * listaProcesos[])
{
    t_PCB * proceso;
    proceso = (t_PCB*)list_remove_element(listaProcesos[estadoActual], encontrarProcesoPorPIDYLista(listaProcesos[estadoActual], idProceso));
    list_add(listaProcesos[estadoSiguiente], proceso);

    timeDifferenceStop(&(proceso->tiempoEnEstado));
    int tiempoASumar = proceso->tiempoEnEstado.mDelta;
    proceso->MT[estadoActual] += tiempoASumar;
    proceso->ME[estadoSiguiente]++;
    timeDifferenceStart(&(proceso->tiempoEnEstado));

    log_info(logger, "## (%d) Pasa del estado %s al estado %s", proceso->PID, estadoAsString(estadoActual), estadoAsString(estadoSiguiente));
    if(estadoSiguiente == EXIT){
        log_info(logger, "## (%d) - Finaliza el proceso", proceso->PID);
        loguearMetricasDeEstado_PorPCB(proceso, listaProcesos);
    }
    if(estadoActual == EXEC)
        proceso->EJC_ACT += tiempoASumar;
    if(estadoActual == EXEC && estadoSiguiente == READY){ // Unico caso es en interrupt
        log_info(logger, "## (%d) Desalojado por algoritmo SJF/SRT", proceso->PID);
    }
}

t_PCB * encontrarProcesoPorPIDYLista(t_list * lista, int pid){
    #ifndef __INTELLISENSE__ // Lo marco asi para que me deje de marcar error. El compilador lo deberia tomar bien a la hora de crear el ejecutable
    bool _PIDCoincide(void * elemento){         // Esto el VSC lo marca como error pero GCC lo permite y esta en el manual de commons/list
        return ((t_PCB*)elemento)->PID == pid;
    }
    return list_find(lista, _PIDCoincide);
    #endif
}

char * estadoAsString(enum estado e){
    switch(e){
        case NEW: 
        return "NEW";
        case READY:
        return "READY";
        case EXEC:
        return "EXEC";
        case EXIT:
        return "EXIT";
        case BLOCKED:
        return "BLOCKED";
        case SUSP_BLOCKED:
        return "SUSP_BLOCKED";
        case SUSP_READY:
        return "SUSP_READY";
        default:
        return "N/A";
    }
}

enum algoritmo algoritmoStringToEnum(char * algoritmo){
    if(!strcmp(algoritmo, "FIFO")){
        return FIFO;
    }
    if(!strcmp(algoritmo, "SJF")){
        return SJF;
    }
    if(!strcmp(algoritmo, "SRT")){
        return SRT;
    }
    if(!strcmp(algoritmo, "PMCP")){
        return PMCP;
    }
    return ERROR_NO_ALGORITMO;
}

void ordenar_cola_ready(t_list * listaProcesos[], enum algoritmo algoritmo){
    switch (algoritmo)
    {
    case FIFO: // Dejar como esta
        break;
    case SJF: // Ordenar por SJF
        //TODO: Checkpoint 3
        break;
    case SRT: // Ordenar por SRT
        //TODO: Checkpoint 3
        break;
    default:
        // ERROR
        break;
    }
    return;
}
int duracionProceso(t_PCB * proceso){ //Valido para procesos que no se esten ejecutando en CPU, ya que EJC_ACT se actualiza al interrumpir
    return proceso->EST - proceso->EJC_ACT;
}
int duracionProcesoEnEjecucion(t_PCB * proceso){
    return proceso->EST - proceso->EJC_ACT - milisegundosDesde(proceso->tiempoEnEstado.inicio); // tiempo actual desde que se cambio al estado en ejecucion
}
void * menorDuracionProcesoEXEC(void *p1, void *p2){
    if(duracionProcesoEnEjecucion(p1) <= duracionProcesoEnEjecucion(p2))
        return p1;
    else 
        return p2;
}
void * menorDuracionProceso(void * p1, void * p2){
    if(duracionProceso(p1) <= duracionProceso(p2))
        return p1;
    else 
        return p2;
}
void * mayorDuracionProcesoEXEC(void *p1, void *p2){
    if(duracionProcesoEnEjecucion(p1) >= duracionProcesoEnEjecucion(p2))
        return p1;
    else 
        return p2;
}
void * mayorDuracionProceso(void * p1, void * p2){
    if(duracionProceso(p1) >= duracionProceso(p2))
        return p1;
    else 
        return p2;
}
t_PCB * procesoMasBreve(t_list * listaProcesos[], enum estado est){
    if(est == EXEC)
        return list_get_minimum(listaProcesos[est], menorDuracionProcesoEXEC);
    else
        return list_get_minimum(listaProcesos[est], menorDuracionProceso);
}

t_PCB * procesoMasDuradero(t_list * listaProcesos[], enum estado est){
    if(est == EXEC)
        return list_get_maximum(listaProcesos[est], menorDuracionProcesoEXEC);
    else
        return list_get_maximum(listaProcesos[est], menorDuracionProceso);
}
t_PCB * procesoADesalojar(t_list * listasProcesos[], enum algoritmo alg){
    if(alg == FIFO || alg == SJF || list_is_empty(listasProcesos[READY])){
        return NULL;
    }else{
        t_PCB * pLargoExec = procesoMasDuradero(listasProcesos, EXEC);
        t_PCB * pBreveReady = procesoMasBreve(listasProcesos, READY);
        if(duracionProceso(pBreveReady)<duracionProcesoEnEjecucion(pLargoExec))
            return pLargoExec;
        else
            return NULL;
    }
}
void actualizarEstimacion(t_PCB * proceso, float alfa){ // Siempre ejecutar luego de cambiar el estado (lo que modifica EJC_ACT al tiempo total de ejecucion)
    proceso->EJC_ANT = proceso->EJC_ACT;
    proceso->EJC_ACT = 0;
    proceso->EST = proceso->EJC_ANT * alfa + proceso->EST * (1-alfa);
}

bool verificarDesalojo(t_list * listasProcesos[]){ // Solo para STR
    bool desalojar = false;
    // Buscar en lista de EXEC si algun proceso tiene menos tiempo que el menor tiempo de Ready
    // Si READY esta vacio: no desalojar (No deberia ocurrir)
    return desalojar;
}

void * procesoMasCorto(void * p1, void * p2){
    if (((t_PCB*)p1)->SIZE <= ((t_PCB*)p2)->SIZE)
        return p1;
    return p2;
}

PeticionesIO * encontrarPeticionesDeIOPorNombre(t_list * lista, char * nombreIO){
    #ifndef __INTELLISENSE__ // Lo marco asi para que me deje de marcar error. El compilador lo deberia tomar bien a la hora de crear el ejecutable
    bool _PIDCoincide(void * elemento){         // Esto el VSC lo marca como error pero GCC lo permite y esta en el manual de commons/list
        return !strcmp(((PeticionesIO*)elemento)->nombre, nombreIO);
    }
    return list_find(lista, _PIDCoincide);
    #endif
}
void encolarPeticionIO(int PID, char * nombreIO, int milisegundos, t_list * lista_peticiones){
    PeticionesIO * io = encontrarPeticionesDeIOPorNombre(lista_peticiones, nombreIO);
    Peticion * peticion = malloc(sizeof(Peticion));
    peticion->PID = PID;
    peticion->milisegundos = milisegundos;
    list_add(io->cola, peticion);
    sem_post(&(io->sem_peticiones));
    return;
}

char * syscallAsString(CODIGO_OP syscall){
    switch(syscall){
        case SYSCALL_DUMP_MEMORY:
        return "DUMP_MEMORY";
        break;
        case SYSCALL_EXIT:
        return "EXIT";
        break;
        case SYSCALL_INIT_PROC:
        return "INIT_PROC";
        break;
        case SYSCALL_IO:
        return "IO";
        break;
        default:
        return "ERROR - No es SYSCALL";
        break;
    }
}

void liberarMemoria(int PID){
    int socketMemoria = conectarSocketClient(conexiones.ipYPuertoMemoria.IP, conexiones.ipYPuertoMemoria.puerto);
    t_paquete * paq = crear_paquete(PROCESO_FINALIZADO_LIBERAR_MEMORIA);
    agregar_a_paquete(paq, &PID, sizeof(PID));
    enviar_paquete(paq, socketMemoria);
    liberarConexion(socketMemoria);
}

void enviarSolicitudDumpMemory(int PID, int * socketMemoria){
    (*socketMemoria) = conectarSocketClient(conexiones.ipYPuertoMemoria.IP, conexiones.ipYPuertoMemoria.puerto);
    t_paquete * paq = crear_paquete(SOLICITUD_MEMORIA_DUMP_MEMORY);
    agregar_a_paquete(paq, &PID, sizeof(PID));
    enviar_paquete(paq, (*socketMemoria));
    // No cierra conexion: requiere confirmacion de que finalizo
}

