#ifndef INSTRUCCION_H
#define INSTRUCCION_H

typedef enum {
    F_NOOP,
    F_READ,
    F_WRITE,
    F_COPY,
    F_IO,
    F_EXIT
} t_identificador_instruccion;

typedef struct {
    t_identificador_instruccion identificador;
    int param1;
    int param2;
    int param3;
} t_instruccion;

#endif