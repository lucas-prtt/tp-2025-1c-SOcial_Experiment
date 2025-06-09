#ifndef TIEMPO_H
#define TIEMPO_H
#include <stdlib.h>
#include <stdio.h>
#include <time.h>



typedef struct t_timeDifference{
    struct timespec inicio;
    struct timespec fin;
    long long int nDelta;
    long long int uDelta;
    long long int mDelta;
} t_timeDifference;

int milisegundosDesde(struct timespec time);
void timeDifferenceStart(t_timeDifference * cantidadDeTiempo);
void timeDifferenceStop(t_timeDifference * cantidadDeTiempo);
char * timestampNow();
#endif