#include <main.h>
t_config* config;
t_log* logger;

int main(int argc, char* argv[]) {
    config = config_create("memory.config");
    if (config == NULL){ 
        abort(); 
    }
    logger = log_create("memory.log", "memory", false, log_level_from_string(config_get_string_value(config, "LOG_LEVEL")));
    
    log_info(logger, "Memoria iniciada");

    // se gurada lista de instrucciones
    instrucciones_por_pid = dictionary_create();

    // ----- HARDCOREADO PARA PRUEBA DEL CHECKPOINT 2 -----
    t_list* lista_instrucciones = list_create();

    t_instruccion* instruccion1 = malloc(sizeof(t_instruccion));
    instruccion1->identificador = F_READ;
    instruccion1->param1 = 1;
    instruccion1->param2 = 0;
    instruccion1->param3 = 0;
    list_add(lista_instrucciones, instruccion1);

    t_instruccion* instruccion2 = malloc(sizeof(t_instruccion));
    instruccion2->identificador = F_WRITE;
    instruccion2->param1 = 1;
    instruccion2->param2 = 0;
    instruccion2->param3 = 42;
    list_add(lista_instrucciones, instruccion2);

    dictionary_put(instrucciones_por_pid, "1", lista_instrucciones);  // PID = 1
    log_info(logger, "Se cargaron instrucciones de prueba para el PID 1");
    // -----------------------------------------------------

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