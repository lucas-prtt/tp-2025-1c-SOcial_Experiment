#include <main.h>

int main(int argc, char* argv[]) {
    abrirConfigYLog("memoria.config", "memoria.log", "memoria", false);
    log_debug(logger, "Memoria iniciada");
    inicializarVariablesGlobales(
    config_get_int_value(config, "ENTRADAS_POR_TABLA"),
    config_get_int_value(config, "CANTIDAD_NIVELES"),
    config_get_int_value(config, "TAM_MEMORIA"),
    config_get_int_value(config, "TAM_PAGINA"),
    config_get_string_value(config, "PATH_INSTRUCCIONES"),
    config_get_string_value(config, "DUMP_PATH"),
    config_get_int_value(config, "RETARDO_MEMORIA"),
    config_get_int_value(config, "RETARDO_SWAP"),
    config_get_string_value(config, "PATH_SWAPFILE")
    );
    borrarSwapfile();
    log_debug(logger, "Variables de config obtenidas");
    int socketServidor = crearSocketConfig(config, "PUERTO_ESCUCHA");
    if (socketServidor == -1) {
        log_error(logger, "No se pudo crear el socket servidor");
        return EXIT_FAILURE;
    }
    log_debug(logger, "Servidor listo en puerto %d", config_get_int_value(config, "PUERTO_ESCUCHA"));

    // Se crea un hilo que acepta conexiones (por el momento no voy a saber desde que modulo se conectan)
    int* socketPtr = malloc(sizeof(int));
    *socketPtr = socketServidor;

    pthread_t hiloServidor;
    pthread_create(&hiloServidor, NULL, aceptarConexiones, socketPtr);
    pthread_detach(hiloServidor);

    pause(); 

    return 0;
}