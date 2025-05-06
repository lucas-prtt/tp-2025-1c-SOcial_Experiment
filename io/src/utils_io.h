#include <commons/log.h>
#include <commons/config.h>
#include <utils/socketUtils.h>
#include <utils/enums.h>
#include <utils/paquetes.h>
#include <utils/handshake.h>


typedef struct {
    int pid;
    int tiempo; //Por ahora segundos
} request_io;

t_log* iniciarLogger(char* nombreArchivo, char* nombreProceso, t_log_level logLevel);
t_config* iniciarConfig(char *nombreArchivo);