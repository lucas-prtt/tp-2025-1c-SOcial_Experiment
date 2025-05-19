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

extern bool hayInterrupcion;
extern pthread_mutex_t mutexInterrupcion;

typedef struct {
    int pid;
    int pc;
    //t_list *instrucciones; //Debe recibir las instrucciones una a una
} PCB_cpu;

enum TIPO_INSTRUCCION {
    INSTR_NOOP,
    INSTR_WRITE,
    INSTR_READ,
    INSTR_GOTO,
    //Las sigueintes instrucciones se consideran syscalls//
    INSTR_IO,
    INSTR_INIT_PROC,
    INSTR_DUMP_MEMORY,
    INSTR_EXIT,
    ERROR_NO_INSTR,
} tipo_de_instruccion;

bool recibirPIDyPC_kernel(int socket_kernel_dispatch, PCB_cpu *proc_AEjecutar);
void ejecutarInstruccion(int socket_memoria, PCB_cpu *proc_AEjecutar, bool *fin_ejecucion);
void pedirInstruccionAMemoria(int socket_memoria, PCB_cpu *proc_AEjecutar) ;
t_list *recibirInstruccionMemoria(int socket_memoria);
enum TIPO_INSTRUCCION interpretarInstruccion(char *nombreInstruccion);
void controlarInterrupciones(void);