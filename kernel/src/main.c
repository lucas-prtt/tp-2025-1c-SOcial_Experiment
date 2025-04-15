#include "main.h"



int main(int argc, char* argv[]) {

    t_config * config = config_create("kernel.config");
    if (config == NULL){ abort(); }
    t_log * logger = log_create("kernel.log", "kernel", false, log_level_from_string(config_get_string_value(config, "LOG_LEVEL")));
    
    log_info(logger, "Kernel iniciado");

    // Soy consciente de que es un lio de llamado de funciones. Despues lo intentaré abstraer y reducir
    // Por lo pronto compila y deberia andar

    int socketCPUDispatch = crearSocketDesdeConfig(config, "PUERTO_ESCUCHA_DISPATCH");
    int socketCPUInterrupt = crearSocketDesdeConfig(config, "PUERTO_ESCUCHA_INTERRUPT");
    int socketEscuchaIO = crearSocketDesdeConfig(config, "PUERTO_ESCUCHA_IO");

    log_debug(logger, "Sockets creados");
    pthread_t cpuDispatchConnect, cpuInterruptConnect, ioConnect;
    pthread_create(&cpuDispatchConnect, NULL, esperarCPUDispatch, &socketCPUDispatch);
    pthread_create(&cpuInterruptConnect, NULL, esperarCPUInterrupt, &socketCPUInterrupt);
    pthread_create(&ioConnect, NULL, esperarIOEscucha, &socketEscuchaIO);
    log_debug(logger, "threads creados");
    printf("Conectando modulos\nPresione enter para finalizar la etapa de conexion y comenzar a ejecutar el kernel");
    getchar();
    threadCancelAndDetach(&cpuDispatchConnect); //Envia un cancel request. No se cancela hasta que se haga shutdown (accept() es bloqueante)
    threadCancelAndDetach(&cpuInterruptConnect);
    threadCancelAndDetach(&ioConnect);
    shutdown(cpuDispatchConnect, SHUT_RD); // RE util: hace que se finalice un accept con return -1
    shutdown(cpuInterruptConnect, SHUT_RD);
    shutdown(ioConnect, SHUT_RD);
    log_debug(logger, "Threads eliminados");

            // A partir de este punto la variable global "conexiones" no la modifica ningun thread 
            // por lo que hay via libre para usarla



    return 0;
}
