#ifndef ENUM_H
#define ENUM_H

typedef enum {
    SOYKERNEL,
    SOYCPU,
    SOYMEMORIA,
    SOYIO
} ID_MODULO;


typedef enum { //CODIGO DE OPERACION
    HANDSHAKE = 1, // Parametros: ID o Nombre (CPU/IO)
    SYSCALL_EXIT = 2, // No necesita parametros
    SYSCALL_INIT_PROC = 3, // 2 Parametros: Nombre_Archivo_Pseudocodigo, Tamaño_Proceso
    SYSCALL_DUMP_MEMORY = 4, // No necesita parametros
    SYSCALL_IO = 5 // 2 Parametros: Nombre_IO, Tiempo_Milisegundos
} CODIGO_OP;


#endif