#ifndef LEERINSTRUCCIONES_H
#define LEERINSTRUCCIONES_H
#include <stdio.h>
#include "utils/logConfig.h"
#include "variablesGlobales.h"
#include "string.h"
#include "utils_memory.h"

int cuantasInstruccionesDelPID(int PID);
char * leerInstruccion(int PID, int PC);
int cargarInstrucciones(int PID, char* PATH, int tamaño);



#endif