#ifndef ENUM_H
#define ENUM_H

typedef enum {
    SOYKERNEL,
    SOYCPU,
    SOYMEMORIA,
    SOYIO
} ID_MODULO;


typedef enum {
    HANDSHAKE = 1,
    PETICION_IO,
    RESPUESTA_PETICION_IO,
    PETICION_EXECUTE,
    //Capaz despues un RESPUESTA_PETICION_EXECUTE?
} CODIGO_OP;


#endif