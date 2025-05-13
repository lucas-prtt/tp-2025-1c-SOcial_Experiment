/*
typedef enum {
    INSTR_NOOP,
    INSTR_WHITE,
    INSTR_READ,
    INSTR_GOTO,
    //Las sigueintes instrucciones se consideran syscalls//
    INSTR_IO,
    INSTR_INIT_PROC,
    INSTR_DUMP_MEMORY,
    INSTR_EXIT,
} TIPO_INSTRUCCION;

*/

NOOP
WRITE 0 EJEMPLO_DE_ENUNCIADO
READ 0 20
GOTO 0
////////las siguientes se concideran syscalls
IO IMPRESORA 25000
INIT_PROC preceso1 256
DUMP_MEMORY
EXIT