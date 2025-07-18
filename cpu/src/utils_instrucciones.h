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
#include "utils_cpu.h"
#include "traducciones.h"


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

void *atenderKernelDispatch(void *cpu_args);
void *atenderKernelInterrupt(void *cpu_args);

bool recibirPIDyPCkernel(int socket_kernel_dispatch, PCB_cpu *proc_AEjecutar, int *estado_conexion);
enum TIPO_INSTRUCCION instrucciones_string_to_enum(char *nombreInstruccion);
void setProgramCounter(PCB_cpu *pcb, int newProgramCounter);
bool ejecutarCicloInstruccion(cpu_t *cpu, PCB_cpu *proc_AEjecutar);
int recibirInterrupcion(int socket_kernel_interrupt);
void devolverProcesoPorInterrupt(int socket_kernel, PCB_cpu *proc_AEjecutar);
void MostrameTodaLaCache(cpu_t * cpu);

char *fetch(int socket_memoria, PCB_cpu *proc_AEjecutar);
t_list *decode(PCB_cpu *proc_AEjecutar, char *instruccion, instruccionInfo *instr_info);
bool execute(cpu_t *cpu, t_list *instruccion_list, instruccionInfo instr_info, PCB_cpu *pcb);
bool checkInterrupt(cpu_t *cpu, PCB_cpu *proc_AEjecutar);