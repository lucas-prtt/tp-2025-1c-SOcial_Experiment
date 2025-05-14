#include "main.h"

int main(int argc, char* argv[]) {
    int identificadorCPU = atoi(argv[1]);
    abrirConfigYLog("cpu.config", "cpu.log", "cpu", false);
    char* ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    char* ip_kernel = config_get_string_value(config, "IP_KERNEL");
    char* puerto_kernel_dispatch = config_get_string_value(config, "PUERTO_KERNEL_DISPATCH");
    char* puerto_kernel_interrupt = config_get_string_value(config, "PUERTO_KERNEL_INTERRUPT");
    log_info(logger, "CPU %d iniciada", identificadorCPU);

    if(argc != 2) {
        log_debug(logger, "Parametros insuficientes para el inicio");
        cerrarCPU();
    }

    int conexionMemoria = conectarSocketClient(ip_memoria, puerto_memoria);
    int conexionKernelDispatch = conectarSocketClient(ip_kernel, puerto_kernel_dispatch);
    int conexionKernelInterrupt = conectarSocketClient(ip_kernel, puerto_kernel_interrupt);

    verificarConexionCliente(conexionMemoria, "Memoria");
    verificarConexionCliente(conexionKernelDispatch, "Kernel (Dispatch)");
    verificarConexionCliente(conexionKernelInterrupt, "Kernel (Interrupt)");

    //bool resultHandshakeMemoria = handshakeClient(conexionMemoria, identificadorCPU);
    bool resultHandshakeKernelDispatch = handshakeClient(conexionKernelDispatch, identificadorCPU);
    bool resultHandshakeKernelInterrupt = handshakeClient(conexionKernelInterrupt, identificadorCPU);

    //verificarResultadoHandshake(resultHandshakeMemoria, "Memoria");
    verificarResultadoHandshake(resultHandshakeKernelDispatch, "Kernel (Dispatch)");
    verificarResultadoHandshake(resultHandshakeKernelInterrupt, "Kernel (Interrupt)");
    

    
    liberarConexion(conexionMemoria);
    liberarConexion(conexionKernelDispatch);
    liberarConexion(conexionKernelInterrupt);
	log_destroy(logger);
	config_destroy(config);
}