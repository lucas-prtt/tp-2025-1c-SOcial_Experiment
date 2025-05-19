#include <commons/log.h>
#include <commons/config.h>
#include <utils/socketUtils.h>
#include <utils/enums.h>
#include <utils/paquetes.h>
#include <utils/logConfig.h>


typedef struct {
    int socket_memoria;
    int socket_kernel_dispatch;
} sockets_dispatcher;

void cerrarCPU(void);
int generarSocket(char* ip_cliente, char* puerto_cliente, char* modulo_cliente);
void verificarConexionCliente(int socket_cliente, char* nombreModuloCliente);
void realizarHandshake(int socket_cliente, int identificadorCPU, char* modulo_cliente);
bool handshakeCliente(int socket_cliente, int identificador);
void verificarResultadoHandshake(bool result, char* nombreModuloCliente);
sockets_dispatcher *prepararSocketsDispatcher(int socket_memoria, int socket_kernel_dispatch);
void *atenderKernelDispatch(void *socket);
void *atenderKernelInterrupt(void *socket);

