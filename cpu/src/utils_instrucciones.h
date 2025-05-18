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
#include "utils_cpu.h"


bool recibirPIDyPC_kernel(int socket_kernel_dispatch, PCB_cpu *proc_AEjecutar);
void controlarInterrupciones(void);