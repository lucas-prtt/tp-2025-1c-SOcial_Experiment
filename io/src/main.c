#include "main.h"

int main(int argc, char* argv[]) {
    int identificadorIO = atoi(argv[1]);

    t_config* config = iniciarConfig("io.config");
    char* ip_kernel = config_get_string_value(config, "IP_KERNEL");
    char* puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    char* logLevel = config_get_string_value(config, "LOG_LEVEL");

    t_log* logger = iniciarLogger("io.log", "io", log_level_from_string(logLevel));
    log_info(logger, "LOGGER INICIADO COMO: %d", identificadorIO);

    int conexion = conectarSocketClient(ip_kernel, puerto_kernel);
    verificarConexionCliente(conexion, logger, "Kernel");

    int resultHandshake = handshakeClient(conexion, identificadorIO);
    verificarResultadoHandshake(resultHandshake, logger, "Kernel");


    /*
    while(true) { //NO estoy seguro, nunca sale del bucle
        request_io request;
        recibirPeticion(conexion, request);
        ejecutarPeticion(logger, request);
        //notificarFinPeticion(conexion, motivo);
    }
    */
    
    liberarConexion(conexion);
	log_destroy(logger);
	config_destroy(config);
}