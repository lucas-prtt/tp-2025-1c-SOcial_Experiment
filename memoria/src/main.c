#include "main.h"
t_config* config;
t_log* logger;

int main(int argc, char* argv[]) {
    config = config_create("memory.config");
    if (config == NULL){ 
        log_error(logger, "No se pudo leer archivo config");
        abort(); 
    }
    logger = log_create("memory.log", "memory", true, log_level_from_string(config_get_string_value(config, "LOG_LEVEL")));

    procesos = list_create();
    
    log_info(logger, "Memoria iniciada");

    int socketServidor = crearSocketConfig(config, "PUERTO_ESCUCHA");
    if (socketServidor == -1) {
        log_error(logger, "No se pudo crear el socket servidor");
        return EXIT_FAILURE;
    }

    log_info(logger, "Servidor listo en puerto %d", config_get_int_value(config, "PUERTO_ESCUCHA"));

    // Se crea un hilo que acepta conexiones (por el momento no voy a saber desde que modulo se conectan)
    int* socketPtr = malloc(sizeof(int));
    *socketPtr = socketServidor;

    pthread_t hiloServidor;
    pthread_create(&hiloServidor, NULL, aceptarConexiones, socketPtr);
    pthread_detach(hiloServidor);

    pause(); 

    return 0;
}