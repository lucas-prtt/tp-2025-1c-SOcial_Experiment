#include "main.h"

/*
Logs obligatorios:

Fetch Instrucción //
Interrupción Recibida //
Instrucción Ejecutada //
Lectura/Escritura Memoria
Obtener Marco
TLB Hit //
TLB Miss //
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
    
    realizarHandshakeMemoria(socket_memoria, identificadorCPU, "Memoria"); //modificar porque recibe de MEMORIA mas cosas en el handshake 
    realizarHandshakeKernel(socket_kernel_dispatch, identificadorCPU, "Kernel (Dispatch)");
    realizarHandshakeKernel(socket_kernel_interrupt, identificadorCPU, "Kernel (INterrupt)");

    cpu_t *args_cpu = prepararCPU(socket_memoria, socket_kernel_dispatch, socket_kernel_interrupt); //hace falta liberar memoria

    pthread_t atender_kernel_D, atender_kernel_I;
    // Hacer un hilo que maneje la conexion entre memoria y cpu, para que no tenga que hacer el handshake mil veces?
    pthread_create(&atender_kernel_D, NULL, atenderKernelDispatch, &args_cpu);
    pthread_create(&atender_kernel_I, NULL, atenderKernelInterrupt, &args_cpu);


    getchar(); //TODO: para que no termine inmediatamente


    threadCancelAndDetach(&atender_kernel_D);
    threadCancelAndDetach(&atender_kernel_I);




    free(args_cpu);
    liberarConexiones(socket_memoria, socket_kernel_dispatch, socket_kernel_interrupt);
	log_destroy(logger);
	config_destroy(config);
}