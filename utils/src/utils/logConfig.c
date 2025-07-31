#include "logConfig.h"
t_log * logger;
t_config * config;
int abrirConfigYLog(char* configName, char* logName, char * processName, bool console){
    config = config_create(configName);
    if (config == NULL) { return 1; }
    logger = log_create(logName, processName, console, log_level_from_string(config_get_string_value(config, "LOG_LEVEL")));
    if (logger == NULL) { config_destroy(config); return 2; }
    log_debug(logger, "================ Log Iniciado ================");
    return 0;
}
int cerrarConfigYLog(){
    if (config == NULL && logger == NULL){
        return 3; // ERROR: NO ESTA NI CONFIG NI LOGGER
    }
    if (config == NULL){
        return 1; // ERROR: NO ESTA EL CONFIG
    }
    if (logger == NULL){
        return 2; // ERROR: NO ESTA EL LOGGER
    }
    config_destroy(config);
    log_debug(logger, "=============== Log Finalizado =============== ");
    log_destroy(logger);
    return 0;
}