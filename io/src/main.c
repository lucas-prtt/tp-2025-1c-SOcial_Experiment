#include "main.h"

int main(int argc, char* argv[]) {
    char* nombreIO = argv[1];

    t_config* config = iniciarConfig("io.config");
    char* ip = config_get_string_value(config, "IP_KERNEL");
    char* puerto = config_get_string_value(config, "PUERTO_KERNEL");
    char* logLevel = config_get_string_value(config, "LOG_LEVEL");

    t_log* logger = iniciarLogger("io.log", "io", log_level_from_string(logLevel));
    log_info(logger, "LOGGER INICIADO COMO: %s", nombreIO);

    int conexion = conectarSocketClient(ip, puerto);

    liberarConexion(conexion);
	log_destroy(logger);
	config_destroy(config);
}