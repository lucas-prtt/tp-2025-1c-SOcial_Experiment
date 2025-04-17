#include "utils_cpu.h"

t_log* iniciarLogger(char* nombreArchivo, char* nombreProceso, t_log_level logLevel) {
    t_log* nuevoLogger = log_create(nombreArchivo, nombreProceso, false, logLevel);
    if(nuevoLogger == NULL) { abort(); }
    return nuevoLogger;
}

t_config* iniciarConfig(char *nombreArchivoConfig) {
    t_config* nuevoConfig = config_create(nombreArchivoConfig);
    if(nuevoConfig == NULL) { abort(); }
    return nuevoConfig;
}