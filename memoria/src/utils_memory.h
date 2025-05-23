#ifndef UTILS_MEMORY_H
#define UTILS_MEMORY_H

#include <stdio.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <utils/socketUtils.h>
#include <utils/enums.h>
#include <arpa/inet.h>
#include "utils/paquetes.h"
#include "atencionKernel.h"
#include "atencionCPU.h"

typedef struct
{
    t_list* socketModulo;
} ModulosConectados;

extern t_log* logger;

int crearSocketConfig(t_config* config, char opcion[]);
void* aceptarConexiones(void* socketPtr); 
void* atenderConexion(void* socketPtr);

#endif