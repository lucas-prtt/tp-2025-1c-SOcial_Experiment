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


bool recibirPIDyPC_kernel(int socket_kernel_dispatch, PCB_cpu *proc_AEjecutar, int *estado_conexion);
bool ejecutarCicloInstruccion(cpu_t *cpu, PCB_cpu *proc_AEjecutar);
void devolverProcesoKernel(int socket_kernel, PCB_cpu *proc_AEjecutar);
char *fetch(int socket_memoria, PCB_cpu *proc_AEjecutar);
t_list *decode(char *instruccion, instruccionInfo *instr_info);
enum TIPO_INSTRUCCION instrucciones_string_to_enum(char *nombreInstruccion);
bool execute(cpu_t *cpu, t_list *instruccion_list, instruccionInfo instr_info, PCB_cpu *pcb);

int traducirDireccionTLB(TLB *tlb, int pid, int direccion_logica);

void setProgramCounter(PCB_cpu *pcb, int newProgramCounter);

bool recibirInterrupcion(int socket_kernel_dispatch);
bool checkInterrupt(cpu_t *cpu);

void *atenderKernelDispatch(void *cpu_args);
void *atenderKernelInterrupt(void *cpu_args);

