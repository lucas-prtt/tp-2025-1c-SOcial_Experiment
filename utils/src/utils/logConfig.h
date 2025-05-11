#ifndef LOGGER_H
#define LOGGER_H

#include "commons/log.h"
#include "commons/config.h"
int abrirConfigYLog(char* configName, char* logName, char * processName, bool console);
int cerrarConfigYLog();
extern t_log * logger; //Debe haber una unica instancia de logger para evitar que escriban en el mismo archivo al mismo tiempo y se superpongan
extern t_config * config; //Ya que logger lo hago global, config tambien, y se puede leer la configuracion de cualquier parte





#endif
