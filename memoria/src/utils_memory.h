#include <stdio.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <utils/socketUtils.h>
#include <utils/enums.h>

typedef struct
{
    t_list* socketModulo;
} ModulosConectados;

extern t_log* logger;

int crearSocketConfig(t_config* config, char opcion[]);
void* aceptarConexiones(void* socketPtr); 
void* atenderConexion(void* socketPtr);