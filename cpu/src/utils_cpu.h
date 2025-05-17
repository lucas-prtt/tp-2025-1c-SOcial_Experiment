#include <commons/log.h>
#include <commons/config.h>
#include <utils/socketUtils.h>
#include <utils/enums.h>
#include <utils/paquetes.h>
#include <utils/logConfig.h>

void cerrarCPU(void);
void verificarConexionCliente(int socket_cliente, char* nombreModuloCliente);
bool handshakeClient(int socket_cliente, int identificador);
void verificarResultadoHandshake(bool result, char* nombreModuloCliente);

