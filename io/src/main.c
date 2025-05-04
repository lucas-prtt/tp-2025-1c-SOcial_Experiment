#include "main.h"

int main(int argc, char* argv[]) {
    int nombreIO = atoi(argv[1]);

    t_config* config = iniciarConfig("io.config");
    char* ip_kernel = config_get_string_value(config, "IP_KERNEL");
    char* puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    char* logLevel = config_get_string_value(config, "LOG_LEVEL");

    t_log* logger = iniciarLogger("io.log", "io", log_level_from_string(logLevel));
    log_info(logger, "LOGGER INICIADO COMO: %d", nombreIO);

    int conexion = conectarSocketClient(ip_kernel, puerto_kernel);
    if(conexion == -1) {
        log_info(logger, "Error al intentar establecer conexión inicial con el modulo Kernel");
        //exit(EXIT_FAILURE);
    }
    else
        log_info(logger, "Conexión inicial con el modulo Kernel exitosamente establecido");

    int result = handshakeKernel(conexion, nombreIO);
    if(result == -1) {
        log_info(logger, "Error al intentar establecer un protocolo de comunicación con el modulo Kernel");
        //exit(EXIT_FAILURE);
    }
    else
        log_info(logger, "Protocolo inicial de comunicación con el modulo Kernel realizado");

    
    liberarConexion(conexion);
	log_destroy(logger);
	config_destroy(config);
}