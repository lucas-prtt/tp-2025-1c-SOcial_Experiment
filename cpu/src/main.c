#include "main.h"

int main(int argc, char* argv[]) {
    int identificadorCPU = atoi(argv[1]);
    
    t_config* config = iniciarConfig("cpu.config");
    char* ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    char* ip_kernel = config_get_string_value(config, "IP_KERNEL");
    char* puerto_kernel_dispatch = config_get_string_value(config, "PUERTO_KERNEL_DISPATCH");
    char* puerto_kernel_interrupt = config_get_string_value(config, "PUERTO_KERNEL_INTERRUPT");
    char* logLevel = config_get_string_value(config, "LOG_LEVEL");

    t_log* logger = iniciarLogger("cpu.log", "cpu", log_level_from_string(logLevel));
    log_info(logger, "CPU %d INICIADA", identificadorCPU);

// conexion a memoria
    int conexionMemoria = conectarSocketClient(ip_memoria, puerto_memoria);
    ID_MODULO soy = SOYCPU;
    send(conexionMemoria, &soy, sizeof(ID_MODULO), 0);
    log_info(logger, "Handshake SOYCPU enviado a Memoria");

// prueba pedido de instruccion
    uint32_t pid = 1;
    uint32_t pc = 0;
    send(conexionMemoria, &pid, sizeof(uint32_t), 0);
    send(conexionMemoria, &pc, sizeof(uint32_t), 0);
    log_info(logger, "Solicitada la instruccion de PID y PC");

    uint32_t tamanio;
    recv(conexionMemoria, &tamanio, sizeof(uint32_t), MSG_WAITALL);

    char* instruccion = malloc(tamanio);
    memset(instruccion, 0, tamanio);
    recv(conexionMemoria, instruccion, tamanio, MSG_WAITALL);
    log_info(logger, "Instruccion recibida");
    free(instruccion);

    int conexionKernelDispatch = conectarSocketClient(ip_kernel, puerto_kernel_dispatch);
    int conexionKernelInterrupt = conectarSocketClient(ip_kernel, puerto_kernel_interrupt);
    
    liberarConexion(conexionMemoria);
    liberarConexion(conexionKernelDispatch);
    liberarConexion(conexionKernelInterrupt);
	log_destroy(logger);
	config_destroy(config);
}