#include "main.h"

int main(int argc, char *argv[]) {
    char *nombreIO = argv[1];

    t_config *config = iniciarConfig("io.config");
    char *ip_kernel = config_get_string_value(config, "IP_KERNEL");
    char *puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    char *logLevel = config_get_string_value(config, "LOG_LEVEL");

    t_log *logger = iniciarLogger("io.log", "io", log_level_from_string(logLevel));
    log_info(logger, "Logger iniciado: %s", nombreIO);

    if (argc != 2) {
        log_debug(logger, "Parametros insuficientes para el inicio");
        cerrarIO();
    }
    
    int conexion = conectarSocketClient(ip_kernel, puerto_kernel);
    verificarConexionCliente(conexion, logger, "Kernel");

    int resultHandshake = handshakeKernel(conexion, nombreIO);
    verificarResultadoHandshake(resultHandshake, logger, "Kernel");

    /*
    //ATENDER PETICIONES DEL KERNEL// va estar activo mientras el kernel este activo, falta notificar correctamente
    while(true) {
        MOTIVO_FIN_IO motivo;
        request_io request;
        recibirPeticion(conexion, request);
        ejecutarPeticion(logger, request);
        notificarMotivoFinPeticion(conexion, motivo);
    }
    */

    liberarConexion(conexion);
    log_destroy(logger);
    config_destroy(config);
}