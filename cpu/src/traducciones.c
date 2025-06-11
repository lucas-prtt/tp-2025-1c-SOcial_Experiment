#include "traducciones.h"


#define TLB_SIZE 4 /////////

/*
    Ninguna de estas funciones estan totalmente implementadas (algunas ni parametro reciben). Estoy probando cosas
*/

/////////////////////////       < CACHÉ >       /////////////////////////



/////////////////////////       < TLB >       /////////////////////////

void inicializarTLB(TLB *tlb) {
    if(TLB_SIZE == 0) {
        log_info(logger, "TLB Deshabilitada");
        return NULL;
    }

    for(int i = 0; i < TLB_SIZE; i++) {
        tlb->entradas[i].validez = 0;
    }

    tlb->proximo = 0;
    tlb->algoritmo = algoritmo_string_to_enum(config_get_string_value(config, "REEMPLAZO_TLB"));
    //puede ser global, porquee es el mismo para todas las cpus
    log_info(logger, "TLB Habilitada");
}

int buscarPaginaTLB(TLB *tlb, int nro_pagina) {
    int pos_registro = buscarIndicePaginaTLB(tlb, nro_pagina);

    if(pos_registro == -1) {
        log_info(logger, "PID: <PID> - TLB MISS - Pagina: %d", nro_pagina);
        return -1; // TLB MISS
    }
    
    log_info(logger, "PID: <PID> - TLB HIT - Pagina: %d", nro_pagina);
    return tlb->entradas[pos_registro].marco; // TLB HIT
}

int buscarIndicePaginaTLB(TLB *tlb, int nro_pagina) {
    for(int i = 0; i < TLB_SIZE; i++) {
        if(tlb->entradas[i].validez && tlb->entradas[i].pagina == nro_pagina)
            return i;
    }

    return -1;
}

void reemplazarEnTLB(TLB *tlb) {
    // Sucede cuando buscarRegistroTLB es -1 (TLB MISS) //
    int indice_victima;

    if(hayEntradaVaciaTLB(tlb, &indice_victima)) {
        // insertarPaginaTLB(tlb, indice_victima);
    }
    
    indice_victima = seleccionarEntradaVictima(tlb);
    limpiarEntradaTLB(tlb, indice_victima);
    // insertarPaginaTLB(tlb, indice_victima);
}

bool hayEntradaVaciaTLB(TLB *tlb, int *indice_victima) {
    for(int i = 0; i < TLB_SIZE; i++) {
        if(tlb->entradas[i].validez == 0) {
            indice_victima = i;
            return true;
        }
    }
}

int seleccionarEntradaVictima(TLB *tlb) {
    int indice_victima = 0;

    switch(tlb->algoritmo)
    {
        case ALG_FIFO:
        {
            indice_victima = tlb->proximo;
            tlb->proximo = (tlb->proximo + 1) % TLB_SIZE;

            return indice_victima;
        }
        case ALG_LRU:
        {
            int menor_uso = tlb->entradas[0].ultimo_uso;

            for(int i = 1; i < TLB_SIZE; i++) {
                if(tlb->entradas[i].ultimo_uso < menor_uso) {
                    menor_uso = tlb->entradas[i].ultimo_uso;
                    indice_victima = i;
                }
            }
            // contador_uso: cada que tlb_hit o reemplazo
            return indice_victima;
        }
        default:
        {
            log_error(logger, "Algoritmo de reemplazo desconocido");
            return 0;
        }
    }
}

void limpiarEntradaTLB(TLB *tlb, int indice_victima) {
    tlb->entradas[indice_victima].pagina = -1;
    tlb->entradas[indice_victima].marco = -1;
    tlb->entradas[indice_victima].ultimo_uso = -1;
    tlb->entradas[indice_victima].validez = 0;
}

void vaciarTLB(r_TLB tlb[]) {
    // Sucede por proceso //
    for(int i = 0; i < TLB_SIZE; i++)
        limpiarEntradaTLB(tlb, i);
}



/////////////////////////       < UTILS >       /////////////////////////

enum TIPO_ALGORITMO_REEMPLAZO algoritmo_string_to_enum(char *nombreAlgoritmo) {
    if (!strcmp(nombreAlgoritmo, "FIFO")) {
        return ALG_FIFO;
    }
    if (!strcmp(nombreAlgoritmo, "LRU")) {
        return ALG_LRU;
    }
    if (!strcmp(nombreAlgoritmo, "CLOCK")) {
        return ALG_CLOCK;
    }
    if (!strcmp(nombreAlgoritmo, "CLOCK-M")) {
        return ALG_CLOCK_M;
    }
    return ERROR_NO_ALG;
}



/////////////////////////       < TRADUCCIÓN >       /////////////////////////


int deDireccionLogicaAfisica(int direccion_logica, int tamanio_pagina, int nro_marco) {
    int offset = desplazamiento(direccion_logica, tamanio_pagina);
    return ((nro_marco * tamanio_pagina) + offset); //tam marco = tam pagina
}

int numeroPagina(int direccion_logica, int tamanio_pagina) {
    return floor(direccion_logica / tamanio_pagina);
}

int entradaNivelX(int numero_pagina) {
    //return floor(nro_página / cant_entradas_tabla ^ (N - X)) % cant_entradas_tabla
}

int desplazamiento(int direccion_logica, int tamanio_pagina) {
    return direccion_logica % tamanio_pagina;
}