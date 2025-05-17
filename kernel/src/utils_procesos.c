#include "utils_procesos.h"




t_PCB * crearPCB(int id, char * path, int size){
    t_PCB * pcb = malloc(sizeof(pcb));
    pcb->PID = id;
    pcb->SIZE = size;
    pcb->PATH = path;
    pcb->EJC_ANT = 0;
    pcb->EST_ACT = 0;
    pcb->EST_ANT = 0;
    pcb->PC = 0;
    for (int i = 0; i<7; i++){
    pcb->ME[i] = 0;
    pcb->MT[i] = 0;
    }
    return pcb;
}

void nuevoProceso(int id, char * path, int size, t_list * listaProcesos[]){
    list_add(listaProcesos[NEW], crearPCB(id, path, size));
    log_info(logger, "(%d) Se crea el proceso - Estado:NEW", id);
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
    log_info(logger, "(%d) Pasa del estado %s al estado %s", idProceso, estadoAsString(estadoActual), estadoAsString(estadoSiguiente));
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
        break;
    case SRT: // Ordenar por SRT
        break;
    default:
        // ERROR
        break;
    }
    return;
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
