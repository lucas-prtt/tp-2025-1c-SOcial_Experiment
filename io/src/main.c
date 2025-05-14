#include "main.h"

int main(int argc, char *argv[]) {
    char *nombreIO = argv[1];
    abrirConfigYLog("io.config", "io.log", "io", false);
    log_debug(logger, "Logger iniciado: %s", nombreIO);
    char *ip_kernel = config_get_string_value(config, "IP_KERNEL");
    char *puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");

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