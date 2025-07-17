#include "main.h"


int main(int argc, char* argv[]) {
    int identificadorCPU = atoi(argv[1]);

    // snprintf() formatea y reserva memoria para la cadena (no considera '\0') //
    char *nombre_log = malloc(snprintf(NULL, 0, "cpu%d.log", identificadorCPU) + 1);
    sprintf(nombre_log, "cpu%d.log", identificadorCPU);
    
    char *nombre_config = malloc(snprintf(NULL, 0, "cpu%d.config", identificadorCPU) + 1);
    sprintf(nombre_config, "cpu%d.config", identificadorCPU);

    if(abrirConfigYLog(nombre_config, nombre_log, "cpu", false)) {
        cerrarConfigYLog();
        exit(EXIT_FAILURE);
    }

    free(nombre_log);
    free(nombre_config);

    char *ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char *puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    char *ip_kernel = config_get_string_value(config, "IP_KERNEL");
    char *puerto_kernel_dispatch = config_get_string_value(config, "PUERTO_KERNEL_DISPATCH");
    char *puerto_kernel_interrupt = config_get_string_value(config, "PUERTO_KERNEL_INTERRUPT");

    if(argc != 2) {
        log_debug(logger, "Parametros insuficientes para el inicio");
        cerrarConfigYLog();
        exit(EXIT_FAILURE);
    }

    int socket_memoria = generarSocket(ip_memoria, puerto_memoria, "Memoria");
    int socket_kernel_dispatch = generarSocket(ip_kernel, puerto_kernel_dispatch, "Kernel (Dispatch)");
    int socket_kernel_interrupt = generarSocket(ip_kernel, puerto_kernel_interrupt, "Kernel (Interrupt)");

    log_debug(logger, "Conectado con kernel en socket dispatch: %d, interrupt: %d", socket_kernel_dispatch, socket_kernel_interrupt);

    realizarHandshakeMemoria(socket_memoria, identificadorCPU, "Memoria");
    realizarHandshakeKernel(socket_kernel_dispatch, identificadorCPU, "Kernel (Dispatch)");
    realizarHandshakeKernel(socket_kernel_interrupt, identificadorCPU, "Kernel (Interrupt)");

    cpu_t *args_cpu = prepararCPU(socket_memoria, socket_kernel_dispatch, socket_kernel_interrupt);

    pthread_t atender_kernel_D, atender_kernel_I;
    pthread_create(&atender_kernel_D, NULL, atenderKernelDispatch, args_cpu);
    pthread_create(&atender_kernel_I, NULL, atenderKernelInterrupt, args_cpu);
    
    pthread_join(atender_kernel_D, NULL);
    pthread_join(atender_kernel_I, NULL);
    
    cerrarCPU(args_cpu);
    liberarConexiones(socket_memoria, socket_kernel_dispatch, socket_kernel_interrupt);
    cerrarConfigYLog();
}