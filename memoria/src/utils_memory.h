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
extern t_config * config;

// int crearSocket(t_config * config, char opcion[]);
void * recibirConexion(ID_MODULO handshake);
void * kernelConnection(void * socket);
void * CPUConnection(void * socket);