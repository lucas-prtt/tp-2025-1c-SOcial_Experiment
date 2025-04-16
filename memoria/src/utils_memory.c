#include "utils_memory.h"

ModulosConectados conexiones;

void * recibirConexion(ID_MODULO handshake){
    t_config* config = config_create("memory.config");
    t_log* logger = log_create("memory.log", "memory", false, log_level_from_string(config_get_string_value(config, "LOG_LEVEL")));
    switch (handshake)
    {
    case SOYKERNEL:
        printf("Hola soy el kernel \n");
        break;
    
    default:
        abort(); break;
    }
}