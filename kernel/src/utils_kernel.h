#include <stdio.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <utils/socketUtils.h>
typedef struct{
    t_list * CPUsDispatch;
    t_list * CPUsInterrupt;
    int IOEscucha;
} conexionesAModulos;

extern conexionesAModulos conexiones;

int crearSocketDesdeConfig(t_config * config, char opcion[]);
void * esperarCPUDispatch(void * socket);
void * esperarCPUInterrupt(void * socket);
void * esperarIOEscucha(void * socket);