#include "leerInstrucciones.h"



int cargarInstrucciones(int PID, char* PATH, int tamaño){
        char * PATHCompleto = malloc(strlen(PATH) + strlen(directorioPseudocodigo) + 1);
        strcpy(PATHCompleto, directorioPseudocodigo);
        strcat(PATHCompleto, PATH);
        FILE* archivo = fopen(PATHCompleto, "r");
        
        if (!archivo) {
            log_error(logger, "No se pudo abrir el archivo de pseudocódigo para el PID %d, en el path %s", PID, PATHCompleto);
            return 1;
        }

        t_list* instrucciones = list_create();
        char* linea = NULL;
        size_t len = 0;
        ssize_t read;

        while ((read = getline(&linea, &len, archivo)) != -1) {
            // Quitar el salto de línea si existe
            if (linea[read - 1] == '\n') {
                linea[read - 1] = '\0';
            }
            list_add(instrucciones, strdup(linea));
        }
        free(linea);
        fclose(archivo);
        pthread_mutex_lock(&mutex_lista_instrucciones);
        agregarInstruccionesAPID(PID, instrucciones);
        pthread_mutex_unlock(&mutex_lista_instrucciones);
        free(PATHCompleto);
        return 0;
}

char * leerInstruccion(int PID, int PC){
    return list_get(obtenerInstruccionesPorPID(PID), PC);
}
int cuantasInstruccionesDelPID(int PID){
    return  list_size(obtenerInstruccionesPorPID(PID));
}
