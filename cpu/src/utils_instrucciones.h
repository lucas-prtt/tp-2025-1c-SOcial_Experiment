#include <commons/collections/list.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils/logConfig.h"
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
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
};

typedef struct {
    enum TIPO_INSTRUCCION tipo_instruccion;
    bool requiere_traduccion;
} instruccionInfo;

bool recibirPIDyPC_kernel(int socket_kernel_dispatch, PCB_cpu *proc_AEjecutar);
void ejecutarCicloInstruccion(int socket_memoria, int socket_kernel, PCB_cpu *proc_AEjecutar, bool *fin_ejecucion);
char* fetch(int socket_memoria, PCB_cpu *proc_AEjecutar);
instruccionInfo decode(char *instruccion);
char *devolverOperacion(char *instruccion);
enum TIPO_INSTRUCCION instrucciones_string_to_enum(char *nombreInstruccion);
void execute(int socket_memoria, int socket_kernel, char *instruccion, instruccionInfo instr_info, PCB_cpu *pcb, bool *fin_ejecucion);
void controlarInterrupciones(void);

