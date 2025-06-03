#include "traducciones.h"

/*
    Ninguna de estas funciones estan totalmente implementadas (algunas ni parametro reciben). Estoy probando cosas
*/

/////////////////////////       < CACHÉ >       /////////////////////////


/////////////////////////       < TLB >       /////////////////////////

void *inicilizarTLB(r_TLB tlb[], int cantidad_entradas_tlb) {
    if(cantidad_entradas_tlb == 0) {
        log_info(logger, "TLB Deshabilitada");
        return NULL;
    }

    for(int i = 0; i < cantidad_entradas_tlb; i++) {
        tlb[i].pagina = -1;
        tlb[i].marco = -1;
        tlb[i].validez = 1;
        tlb[i].timestamp = 0;
    }

    log_info(logger, "TLB Habilitada");
}

int buscarPaginaTLB(r_TLB tlb[], int cantidad_entradas_tlb, int nro_pagina) {
    // Devuelve la posicion de la pagina buscada en el vector. En caso de no encontrarla devuelve -1 //

    for(int i = 0; i < cantidad_entradas_tlb; i++) { // si la encuentra actualiza el timestamp
        if(tlb[i].pagina == nro_pagina) {}
            return i;
    }

    return -1;
}

// o mejor buscarMarcoTLB
int buscarRegistroTLB(r_TLB tlb[], int cantidad_entradas_tlb, int nro_pagina) {
    // Devuelve TLB_MISS si no encuentra el registro. Si lo encuentra devuelve el marco //

    int pos_registro = buscarPaginaTLB(tlb, cantidad_entradas_tlb, nro_pagina);

    if(pos_registro == -1) {
        log_info(logger, "PID: <PID> - TLB MISS - Pagina: %d", nro_pagina);
        return TLB_MISS;
    }
    
    log_info(logger, "PID: <PID> - TLB HIT - Pagina: %d", nro_pagina);
    //el times stamp vuelve a cero
    return tlb[pos_registro].marco;
}

/*
void reemplazarEnTLB() {
    char *algoritmo = config_get_string_value(config, "REEMPLAZO_TLB"); //como arg, no lo voy a hacer siempre
    TIPO_ALGORITMO_REMPLAZO algoritmo_reemplazo_tlb = algoritmo_string_to_enum(algoritmo)
    
    int victima; //mas como las pos en la tlb
    victima = seleccionarEntradaVictima(tlb, algoritmo_reemplazo); //puedo ser la pos o en caso de la lsita el puntero al nodo.abrirConfigYLog

    limpiarEntradaTLB(pos);

    actualizarTimestamps(); //antes para que no sume uno al recien ingresado

    insertarPaginaTLB(pos, );
}

void actualizarTimestamps(void) {
    // pasa por cada registro de la tlb y aumenta sus timestamp (menos al del recien ingresado)
    for(int i = 0; i < cantidad_entradas_tlb; i++) // para los que esten disponibles.
        tlb[i].timestamp++;
}

*/
