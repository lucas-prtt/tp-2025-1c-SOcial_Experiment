#include <main.h>

int main(int argc, char* argv[]) {
    abrirConfigYLog("memory.config", "memory.log", "memory", false);
    log_info(logger, "Memoria iniciada");
    inicializarVariablesGlobales(
    config_get_int_value(config, "ENTRADAS_POR_TABLA"),
    config_get_int_value(config, "CANTIDAD_NIVELES"),
    config_get_int_value(config, "TAM_MEMORIA"),
    config_get_int_value(config, "TAM_PAGINA"),
    config_get_string_value(config, "PATH_INSTRUCCIONES"),
    config_get_string_value(config, "PATH_DUMP"),
    config_get_int_value(config, "RETARDO_MEMORIA"),
    config_get_int_value(config, "RETARDO_SWAP")
    );

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