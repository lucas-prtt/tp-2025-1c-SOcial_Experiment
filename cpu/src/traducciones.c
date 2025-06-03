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
