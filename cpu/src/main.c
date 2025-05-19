#include "main.h"

int main(int argc, char* argv[]) {
    int identificadorCPU = atoi(argv[1]);

    char* nombre_log = malloc(snprintf(NULL, 0, "cpu_%d.log", identificadorCPU) + 1); //snprintf() devuelve el tamaño de cpu_%d.log (no considera '\0')//
    sprintf(nombre_log, "cpu_%d.log", identificadorCPU);
    abrirConfigYLog("cpu.config", nombre_log, "cpu", false);

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
    int *socket_kernel_dispatch = malloc(sizeof(int)); *socket_kernel_dispatch = generarSocket(ip_kernel, puerto_kernel_dispatch, "Kernel (Dispatch)");
    int *socket_kernel_interrupt = malloc(sizeof(int)); *socket_kernel_interrupt = generarSocket(ip_kernel, puerto_kernel_interrupt, "Kernel (Interrupt)");
    
    
    //realizarHandshake(socket_memoria, identificadorCPU, "Memoria");
    realizarHandshake(*socket_kernel_dispatch, identificadorCPU, "Kernel (Dispatch)");
    realizarHandshake(*socket_kernel_interrupt, identificadorCPU, "Kernel (INterrupt)");

    sockets_dispatcher *sockets_for_dispatch = prepararSocketsDispatcher(socket_memoria, *socket_kernel_dispatch);
    
    pthread_t atenderKernel_D, atenderKernel_I; //crear un hilo para pedir a memoria
    pthread_create(&atenderKernel_D, NULL, atenderKernelDispatch, sockets_for_dispatch);
    pthread_create(&atenderKernel_I, NULL, atenderKernelInterrupt, socket_kernel_interrupt);


    //getchar(); //TODO


    threadCancelAndDetach(&atenderKernel_D);
    threadCancelAndDetach(&atenderKernel_I);





    liberarConexion(socket_memoria);
    liberarConexion(*socket_kernel_dispatch);
    liberarConexion(*socket_kernel_interrupt);
    //Capaz deberian ir en cerrarCPU()//
	log_destroy(logger);
    free(nombre_log);
	config_destroy(config);
}