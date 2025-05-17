#include "tiempo.h"
#include <time.h>

// USO: 
// t_timeDifference diferencia;
// timeDifferenceStart(&diferencia);
// Ejecucion que tarda tiempo
// timeDifferenceStop(&diferencia);
// printf("Diferencia de tiempo en ns = %lld", diferencia.ndelta);

// Realmente "simula" un cronometro al registrar el tiempo de inicio y tiempo de fin y calcular la diferencia
// Es por esto que no importa si haces el start y no el stop, pues no corre nada de fondo


void calcularDiferencia(t_timeDifference * ct){
    ct->nDelta = ct->fin.tv_nsec + ct->fin.tv_sec * 1000000000 - ct->inicio.tv_nsec - ct->inicio.tv_sec * 1000000000;
    ct->uDelta = ct->nDelta / 1000;
    ct->mDelta = ct->uDelta / 1000;
}
void timeDifferenceStart(t_timeDifference * cantidadDeTiempo){
    clock_gettime(CLOCK_MONOTONIC, &(cantidadDeTiempo->inicio));
}
void timeDifferenceStop(t_timeDifference * cantidadDeTiempo){
    clock_gettime(CLOCK_MONOTONIC, &(cantidadDeTiempo->fin));
    calcularDiferencia(cantidadDeTiempo);
}




