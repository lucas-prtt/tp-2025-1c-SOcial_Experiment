#ifndef PROCESOS_H
#define PROCESOS_H

enum estado{ // Para indicar el estado dentro del vector de ME y MT
    NEW = 0,
    READY = 1,
    EXEC = 2,
    EXIT = 3,
    BLOCKED = 4,
    SUSP_BLOCKED = 5,
    SUSP_READY = 6
};

typedef struct{
    int PID;
    int PC;
    int ME[7];
    int MT[7]; // Equivale a aproximadamente 25 dias en miilisegundos por cada estado. Se puede pasar a unsigned long long int para 580 millones de años, pero para esto me parece irrelevante
    int EST_ANT; // Milisegundos de la estimacion anterior
    int EST_ACT; // Milisegundos de la estimacion actual
    int EJC_ANT; // Milisegundos de la ejecucion anterior
} t_PCB;

char * estadoAsString(enum estado);

#endif