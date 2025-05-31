#include "main.h"

/*
Logs obligatorios:

Fetch Instrucción.
Interrupción Recibida
Instrucción Ejecutada
Lectura/Escritura Memoria
Obtener Marco
TLB Hit
TLB Miss
Página encontrada en Caché
Página faltante en Caché
Página ingresada en Caché
Página Actualizada de Caché a Memoria
*/

int main(int argc, char* argv[]) {
    int identificadorCPU = atoi(argv[1]);

    // snprintf() formatea y reserva memoria para la cadena (no considera '\0') //
    char* nombre_log = malloc(snprintf(NULL, 0, "cpu_%d.log", identificadorCPU) + 1);
    sprintf(nombre_log, "cpu_%d.log", identificadorCPU);
    
    if(abrirConfigYLog("cpu.config", nombre_log, "cpu", false)) {
        abort();
    }

    free(nombre_log);

    char* ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    char* ip_kernel = config_get_string_value(config, "IP_KERNEL");
    char* puerto_kernel_dispatch = config_get_string_value(config, "PUERTO_KERNEL_DISPATCH");
    char* puerto_kernel_interrupt = config_get_string_value(config, "PUERTO_KERNEL_INTERRUPT");

    if(argc != 2) {
        log_debug(logger, "Parametros insuficientes para el inicio");
        cerrarCPU();
    }

    int socket_memoria = generarSocket(ip_memoria, puerto_memoria, "Memoria");
    int socket_kernel_dispatch = generarSocket(ip_kernel, puerto_kernel_dispatch, "Kernel (Dispatch)");
    int socket_kernel_interrupt = generarSocket(ip_kernel, puerto_kernel_interrupt, "Kernel (Interrupt)");
    
    realizarHandshake(socket_memoria, identificadorCPU, "Memoria");
    realizarHandshake(socket_kernel_dispatch, identificadorCPU, "Kernel (Dispatch)");
    realizarHandshake(socket_kernel_interrupt, identificadorCPU, "Kernel (INterrupt)");

    sockets_dispatcher *sockets_for_dispatch = prepararSocketsDispatcher(socket_memoria, socket_kernel_dispatch);
    // Memoria liberada en atenderKernelDispatch() //
    
    pthread_t atenderKernel_D, atenderKernel_I; //TODO: crear un hilo para pedir a memoria
    pthread_create(&atenderKernel_D, NULL, atenderKernelDispatch, &sockets_for_dispatch);
    pthread_create(&atenderKernel_I, NULL, atenderKernelInterrupt, &socket_kernel_interrupt);


    getchar(); //TODO: para que no termina inmediatamente


    threadCancelAndDetach(&atenderKernel_D);
    threadCancelAndDetach(&atenderKernel_I);




    
    liberarConexiones(socket_memoria, socket_kernel_dispatch, socket_kernel_interrupt);
	log_destroy(logger);
	config_destroy(config);
}