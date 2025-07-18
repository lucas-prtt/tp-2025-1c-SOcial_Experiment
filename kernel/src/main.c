#include "main.h"

/*
Logs:
Syscall                -HECHO
Crear proceso          -HECHO
Cambio estado          -HECHO
Motivo Bloqueo         -HECHO
Fin IO                 -HECHO
Desalojo SJF/SRT
Fin Proceso            -HECHO
Metricas de estado     -HECHO
*/

int main(int argc, char* argv[]) {
    if(abrirConfigYLog("kernel.config", "kernel.log", "kernel", false)){ // Si se ejecuta con exito, devuelve 0
        abort();
    }    
    
    if (argc != 3) {
        log_debug(logger, "Parametros insuficientes para el inicio");
        cerrarKernel();
    }

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
    hilos = list_create();
    procesos_c_inicializarVariables();
    
    getchar();
    
    // RE util: hace que se finalice un accept con return -1
    shutdown(socketCPUDispatch, SHUT_RD); 
    shutdown(socketCPUInterrupt, SHUT_RD);
    pthread_join(cpuDispatchConnect, NULL);
    pthread_join(cpuInterruptConnect, NULL);
    log_debug(logger, "Threads de conexion eliminados");

    // TODO: Filtrar lista de CPUs con CPUs que no tengan ID (No hicieron handshake, ID = -1)
    // TODO: Filtrar lista de CPUs con CPUs que tengan un tipo de socket (interrupt/dispatch) y no el otro (Uno de los dos handshakes falló)

    bool isConexionIOyCPUDisponible = !list_is_empty(conexiones.CPUsDispatch) && list_size(conexiones.CPUsDispatch) == list_size(conexiones.CPUsInterrupt) && !list_is_empty(conexiones.IOEscucha);
    bool isMemoriaDisponible = verificarModuloMemoriaDisponible();
    bool listoParaIniciar = isMemoriaDisponible /*&& isConexionIOyCPUDisponible*/;
    log_debug(logger, "Verificacion de conexiones realizada");
    
    if(/*!*/listoParaIniciar) {
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
    

    pthread_t var_orderThread, var_ingresoAReadyThread;
    pthread_create(&var_orderThread, NULL, orderThread, NULL);
    pthread_create(&var_ingresoAReadyThread, NULL, ingresoAReadyThread, NULL);
    int cantidadDeIos = list_size(conexiones.IOEscucha);
    int cantidadDeCpus = list_size(conexiones.CPUsDispatch); // = conexiones a interrupt
    log_debug(logger, "Cantidad de IOs: %d, Cantidad de CPUs: %d", cantidadDeIos, cantidadDeCpus);
    generarHilos(hilos, cantidadDeCpus, dispatcherThread, conexiones.CPUsDispatch);
    nuevoProceso(0, argv[1], atoi(argv[2]), listasProcesos); // Agrega el proceso indicado por consola a la lista NEW
    post_sem_introducirAReady();
    sem_init(&evaluarFinKernel, 0, 0);
    while(true){
        sem_wait(&evaluarFinKernel);
        log_debug(logger, "Un proceso se murio! Puedo finalizar?");
        int procesosRestantes = getProcesosMolestando();
        log_debug(logger, "ProcesosRestantes = %d", procesosRestantes);
        if(procesosRestantes == 0){
            log_debug(logger, "Finalizo");
            break;
        }
        log_debug(logger, "Debo seguir ejecutando");
    }
    log_debug(logger, "========== FIN EXISTOSO =========");
    sem_destroy(&evaluarFinKernel);
    eliminarHilos(hilos);
    list_destroy_and_destroy_elements(hilos, free);

    return 0;
}
