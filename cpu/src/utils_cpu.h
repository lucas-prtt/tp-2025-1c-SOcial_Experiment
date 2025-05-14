#include <commons/log.h>
#include <commons/config.h>
#include <utils/socketUtils.h>
#include <utils/enums.h>
#include <utils/paquetes.h>
#include <utils/logConfig.h>

t_log* iniciarLogger(char* nombreArchivo, char* nombreProceso, t_log_level logLevel);
t_config* iniciarConfig(char *nombreArchivo);
void cerrarCPU(void);
int handshakeClient(int socket_cliente, int identificador);

