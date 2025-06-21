#ifndef UTILS_CPU_H
#define UTILS_CPU_H

#include <commons/log.h>
#include <commons/config.h>
#include <utils/socketUtils.h>
#include <utils/enums.h>
#include <utils/paquetes.h>
#include <utils/logConfig.h>
#include "traducciones.h"



typedef struct {
    int socket_memoria;
    int socket_kernel_dispatch;
    int socket_kernel_interrupt;

    CACHE *cache;
    TLB *tlb;

    bool hay_interrupcion;
    pthread_mutex_t mutex_interrupcion;
} cpu_t;



int generarSocket(char* ip_cliente, char* puerto_cliente, char* modulo_cliente);
void verificarConexionCliente(int socket_cliente, char* nombreModuloCliente);

void realizarHandshakeMemoria(int socket_cliente, int identificadorCPU, char* modulo_cliente);
bool handshakeMemoria(int socket_memoria, int identificador);

void realizarHandshakeKernel(int socket_cliente, int identificadorCPU, char* modulo_cliente);
bool handshakeKernel(int socket_kernel, int identificador);

void verificarResultadoHandshake(bool result, char* nombreModuloCliente);

void cerrarCPU(cpu_t *args_cpu);
cpu_t *prepararCPU(int socket_memoria, int socket_kernel_dispatch, int socket_kernel_interrupt);
void liberarConexiones(int socket_memoria, int socket_kernel_dispatch, int socket_kernel_interrupt);

#endif