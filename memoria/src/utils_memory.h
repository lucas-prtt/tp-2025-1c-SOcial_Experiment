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

typedef struct { // estructura para almacenar instrucciones por proceso (PID)
    uint32_t pid;
    t_list* instrucciones; // Lista de strings
} Proceso;

extern t_list* procesos; // Lista global de procesos