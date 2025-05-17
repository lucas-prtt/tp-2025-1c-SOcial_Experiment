#include "main.h"

int main(int argc, char *argv[]) {
    char *nombreIO = argv[1];
    
    abrirConfigYLog("io.config", "io.log", "io", false);
    log_debug(logger, "IO: %s - Iniciada", nombreIO);
    
    char *ip_kernel = config_get_string_value(config, "IP_KERNEL");
    char *puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");

    if (argc != 2) {
        log_debug(logger, "Parametros insuficientes para el inicio");
        cerrarIO();
    }
    
    int conexion = conectarSocketClient(ip_kernel, puerto_kernel);
    verificarConexionKernel(conexion);

    int resultHandshake = handshakeKernel(conexion, nombreIO);
    verificarResultadoHandshake_Kernel(resultHandshake);
    
    while(true) {
        MOTIVO_FIN_IO motivo;
        request_io request;
        if(recibirPeticion(conexion, &request)) {
            ejecutarPeticion(&request, &motivo);
            notificarMotivoFinPeticion(conexion, motivo);
        }
        else break;
    }

    liberarConexion(conexion);
    log_destroy(logger);
    config_destroy(config);
}