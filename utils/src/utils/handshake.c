#include <utils/handshake.h>

void verificarResultadoHandshake(int result, t_log *logger, char *nombreModuloCliente) {
    //verifica el handshake, y notifica con log. Por ahora no sale
    if(result == -1)
        log_info(logger, "Error al intentar establecer un protocolo de comunicación con el modulo %s", nombreModuloCliente);
    else
        log_info(logger, "Protocolo inicial de comunicación con el modulo %s realizado", nombreModuloCliente);
}