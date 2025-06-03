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

// mas como un seleccionar entrada => primero se fija si hay una vacia, si la hay pasa esa. Sino lentra al switch

//int seleccionarEntradaVictima() que llama un seleccionar que se fija si esta vacio y a la que aplica el algoritmo.

// int seleccionarEntradaConAlgoritmo()
int seleccionarEntradaVictima(r_TLB tlb[], int cantidad_entradas_tlb, int algoritmo) {
    // se puede pensar fifo con un indice circular, para mas eficiencia.
    int indice_victima = 0;

    switch(algoritmo)
    {
        case ALG_FIFO:
        {
            int mayor_timestamp = -1;
            for(int i = 0; i < cantidad_entradas_tlb; i++) { // cambiar logica
                if(tlb[i].timestamp > mayor_timestamp) {
                    mayor_timestamp = tlb[i].timestamp;
                    indice_victima = i;
                }
            }
            break;
        }
        case ALG_LRU:
        {
            int menor_timestamp = -1;
            for(int i = 0; i < cantidad_entradas_tlb; i++) {
                if (tlb[i].timestamp < menor_timestamp) {
                    menor_timestamp = tlb[i].timestamp;
                    indice_victima = i;
                }
            }
            break;
        }
    }

    return indice_victima;
}

void vaciarTLB(r_TLB tlb[]) { //sucede por proceso
    int cantidad_entradas_tlb = atoi(config_get_string_value(config, "ENTRADAS_TLB"));
    
    for(int i = 0; i < cantidad_entradas_tlb; i++) {
        r_TLB registro = tlb[i];
        limpiarEntradaTLB(&registro);
    }
}
