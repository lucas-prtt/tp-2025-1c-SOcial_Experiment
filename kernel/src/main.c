#include "main.h"

/*
Logs:
Syscall
Crear proceso          -HECHO
Cambio estado          -HECHO
Motivo Bloqueo
Fin IO
Desalojo SJF/SRT
Fin Proceso
Metricas de estado
*/

int main(int argc, char* argv[]) {
    if(abrirConfigYLog("kernel.config", "kernel.log", "kernel", false)){ // Si se ejecuta con exito, devuelve 0
        abort();
    }    
    /*
    if (argc != 3) {
        log_debug(logger, "Parametros insuficientes para el inicio");
        cerrarKernel();
    } */

    int socketCPUDispatch = crearSocketDesdeConfig(config, "PUERTO_ESCUCHA_DISPATCH");
    int socketCPUInterrupt = crearSocketDesdeConfig(config, "PUERTO_ESCUCHA_INTERRUPT");
    int socketEscuchaIO = crearSocketDesdeConfig(config, "PUERTO_ESCUCHA_IO");
    log_debug(logger, "Sockets creados");

    conexiones.ipYPuertoMemoria.puerto = config_get_string_value(config, "PUERTO_MEMORIA");
    conexiones.ipYPuertoMemoria.IP = config_get_string_value(config, "IP_MEMORIA");
    log_debug(logger, "Datos de Memoria leidos");

    pthread_t cpuDispatchConnect, cpuInterruptConnect, ioConnect;
    pthread_create(&cpuDispatchConnect, NULL, esperarCPUDispatch, &socketCPUDispatch);
    pthread_create(&cpuInterruptConnect, NULL, esperarCPUInterrupt, &socketCPUInterrupt);
    pthread_create(&ioConnect, NULL, esperarIOEscucha, &socketEscuchaIO);
    log_debug(logger, "Threads de conexion creados");

    getchar();

    threadCancelAndDetach(&cpuDispatchConnect); //Envia un cancel request. No se cancela hasta que se haga shutdown (accept() es bloqueante)
    threadCancelAndDetach(&cpuInterruptConnect);
    threadCancelAndDetach(&ioConnect);
    shutdown(cpuDispatchConnect, SHUT_RD); // RE util: hace que se finalice un accept con return -1
    shutdown(cpuInterruptConnect, SHUT_RD);
    shutdown(ioConnect, SHUT_RD);
    log_debug(logger, "Threads de conexion eliminados");

    // TODO: Filtrar lista de CPUs con CPUs que no tengan ID (No hicieron handshake, ID = -1)
    // TODO: Filtrar lista de CPUs con CPUs que tengan un tipo de socket (interrupt/dispatch) y no el otro (Uno de los dos handshakes falló)

    bool isConexionIOyCPUDisponible = !list_is_empty(conexiones.CPUsDispatch) && list_size(conexiones.CPUsDispatch) == list_size(conexiones.CPUsInterrupt) && !list_is_empty(conexiones.IOEscucha);
    bool isMemoriaDisponible = verificarModuloMemoriaDisponible();
    bool listoParaIniciar = isConexionIOyCPUDisponible && isMemoriaDisponible;
    log_debug(logger, "Verificacion de conexiones realizada");
    
    if(!listoParaIniciar) {
        log_debug(logger, "Conexiones insuficientes");
        if(!isMemoriaDisponible) {
            log_debug(logger, "-> Memoria no disponible");
        }
        if(!isConexionIOyCPUDisponible) {
            log_debug(logger, "-> IO o CPU no disponible");
        }
        eliminarConexiones();
        log_debug(logger, "Conexiones Eliminadas");
        log_debug(logger, "Abortando kernel");
        cerrarKernel();
    }
    log_debug(logger, "Finalizacion etapa de conexiones exitosa");

            // A partir de este punto la variable global "conexiones" no la modifica ningun thread 
            // por lo que hay via libre para usarla
    procesos_c_inicializarVariables();
    nuevoProceso(0, argv[1], atoi(argv[2]), listasProcesos); // Agrega el proceso indicado por consola a la lista NEW
    
    return 0;
}
