#include <commons/collections/list.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils/logConfig.h"
#include <string.h>
#include <semaphore.h>
#include "utils/tiempo.h"
#include "utils/enums.h"
#include <sys/socket.h>
#include <utils/threads.h>
#include <utils/paquetes.h>

typedef struct {
    int pid;
    int pc;
} PIDyPC_instr;

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


void *atenderKernelDispatch(void *socket);
bool recibirPIDyPC_kernel(int socket_kernel_dispatch, PIDyPC_instr *proc_AEjecutar);
void ejecutarInstruccion(PIDyPC_instr *proc_AEjecutar, bool fin_ejecucion);
void *atenderKernelInterrupt(void *socket);

/*
NOOP
WRITE 0 EJEMPLO_DE_ENUNCIADO
READ 0 20
GOTO 0
////////las siguientes se concideran syscalls
IO IMPRESORA 25000
INIT_PROC preceso1 256
DUMP_MEMORY
EXIT

*/