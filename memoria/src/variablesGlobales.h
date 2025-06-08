#ifndef VARIABLES_GLOBALES_H
#define VARIABLES_GLOBALES_H
#include "commons/collections/list.h"
#include <stdio.h>
#include <stdlib.h>
typedef struct PIDyTablaDePaginas{
    int PID;
    void** TP;
} PIDyTP;


extern void* memoriaDeUsuario;
extern t_list* tablaDeProcesos; // Llena de PIDyTablaDePaginas

#endif