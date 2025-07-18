#ifndef UTILSKERNEL_H
#define UTILSKERNEL_H

#include <stdio.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <utils/socketUtils.h>
#include <utils/enums.h>
#include <utils/threads.h>
#include <utils/paquetes.h>
#include <utils/logConfig.h>
#include <semaphore.h>

typedef struct {
    char* puerto;
    char* IP;
} IPyPuerto;

typedef struct {
    int ID;
    int SOCKET;
} IDySocket_CPU;

typedef struct {
    char* NOMBRE;
    int SOCKET;
} NombreySocket_IO;

typedef struct {
    t_list * CPUsDispatch;
    t_list * CPUsInterrupt;
    t_list * IOEscucha;
    IPyPuerto ipYPuertoMemoria;
} conexionesAModulos;
extern t_list * hilos;
extern sem_t evaluarFinKernel;
extern conexionesAModulos conexiones; //Muy usada por hilos: Conviene que sea global
void * esperarCPUDispatch(void * socket);
void * esperarCPUInterrupt(void * socket);
void * esperarIOEscucha(void * socket);
int verificarModuloMemoriaDisponible();
void eliminarConexiones(void);
int crearSocketDesdeConfig(t_config * config, char opcion[]);
void generarHilos(t_list * hilos, int cantidad, void * func(void *), t_list * parametros);
void eliminarHilos(t_list * hilos);
IDySocket_CPU * buscarCPUInterruptPorID(int id);
void * IOThread(void * NOMBREYSOCKETIO);

void *handshakeCPUInterrupt(void * socket);
void *handshakeCPUDispatch(void * socket);
void * handshakeIO(void * socket);

void cerrarKernel();
int handshakeMemoria(int socketMemoria);




#endif