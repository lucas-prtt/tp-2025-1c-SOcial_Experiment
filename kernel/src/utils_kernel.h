#include <stdio.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <utils/socketUtils.h>
#include <utils/threads.h>

typedef struct {
    char* puerto;
    char* IP;
} IPyPuerto;

typedef struct {
    int ID;
    int SOCKET;
} IDySocket;

typedef struct {
    t_list * CPUsDispatch;
    t_list * CPUsInterrupt;
    IDySocket IOEscucha;
    IPyPuerto ipYPuertoMemoria;
} conexionesAModulos;


extern conexionesAModulos conexiones;

void * esperarCPUDispatch(void * socket);
void * esperarCPUInterrupt(void * socket);
void * esperarIOEscucha(void * socket);
int verificarModuloMemoriaDisponible();
void eliminarConexiones(void);
int crearSocketDesdeConfig(t_config * config, char opcion[]);

void *handshakeCPUInterrupt(void * socket);
void *handshakeCPUDispatch(void * socket);
void * handshakeIO(void * socket);
