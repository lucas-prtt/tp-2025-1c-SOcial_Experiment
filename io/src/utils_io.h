#include <commons/log.h>
#include <commons/config.h>
#include <utils/socketUtils.h>
#include <utils/enums.h>
#include <utils/paquetes.h>
#include "utils/logConfig.h"


typedef struct {
    int pid;
    int tiempo;
} request_io;

typedef enum {
    IO_SUCCESS,
    IO_DESCONNECTED,
} MOTIVO_FIN_IO;

t_log* iniciarLogger(char* nombreArchivo, char* nombreProceso, t_log_level logLevel);
t_config* iniciarConfig(char *nombreArchivo);
void cerrarIO(void);
void verificarConexionKernel(int socket_cliente);
bool handshakeKernel(int socket_kernel, char* nombre);
void verificarResultadoHandshake_Kernel(bool result);

/*
int recibirPeticion(int socket_kernel, request_io &request);

void ejecutarPeticion(t_log *logger, request_io request);

void notificarMotivoFinPeticion(int socket_kernel, MOTIVO_FIN_IO motivo);
*/