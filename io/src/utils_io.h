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

typedef enum { //Capaz deberia estar en otro lugar
    IO_SUCCESS,
} MOTIVO_FIN_IO;

void cerrarIO(void);
void verificarConexionKernel(int socket_cliente);
bool handshakeKernel(int socket_kernel, char* nombre);
void verificarResultadoHandshake_Kernel(bool result);
bool recibirPeticion(int socket_kernel, request_io *request);
void ejecutarPeticion(request_io *request, MOTIVO_FIN_IO *motivo);
void notificarMotivoFinPeticion(int socket_kernel, MOTIVO_FIN_IO motivo);