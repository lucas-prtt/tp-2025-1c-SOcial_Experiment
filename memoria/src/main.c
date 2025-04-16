#include <main.h>

int main(int argc, char* argv[]) {

    t_config* config = config_create("memory.config");
    if (config == NULL){ abort(); }
    t_log* logger = log_create("memory.log", "memory", false, log_level_from_string(config_get_string_value(config, "LOG_LEVEL")));
    log_info(logger, "Memoria iniciada");
    recibirConexion(SOYKERNEL);

    return 0;
}
