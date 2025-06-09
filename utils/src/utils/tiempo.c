#include "tiempo.h"

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

int milisegundosDesde(struct timespec time){
    struct timespec ahora;
    clock_gettime(CLOCK_MONOTONIC, &ahora);
    return (ahora.tv_nsec + ahora.tv_sec * 1000000000 - time.tv_nsec - time.tv_sec * 1000000000)/1000000;
}
    
void timeDifferenceStart(t_timeDifference * cantidadDeTiempo){
    clock_gettime(CLOCK_MONOTONIC, &(cantidadDeTiempo->inicio));
}
void timeDifferenceStop(t_timeDifference * cantidadDeTiempo){
    clock_gettime(CLOCK_MONOTONIC, &(cantidadDeTiempo->fin));
    calcularDiferencia(cantidadDeTiempo);
}


char * timestampNow() {
    struct timespec tiempo;
    struct tm *tiempoDescompuesto;
    static char buffer[13]; // HH:MM:SS:mss0
    clock_gettime(CLOCK_REALTIME, &tiempo);
    tiempoDescompuesto = localtime(&tiempo.tv_sec);
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d:%03ld", tiempoDescompuesto->tm_hour, tiempoDescompuesto->tm_min, tiempoDescompuesto->tm_sec, tiempo.tv_nsec / 1000000);
    return buffer;
}




