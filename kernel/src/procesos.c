#include "procesos.h"




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