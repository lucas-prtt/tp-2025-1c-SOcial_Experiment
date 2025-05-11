#include "procesos.h"




char * estadoAsString(int e){
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