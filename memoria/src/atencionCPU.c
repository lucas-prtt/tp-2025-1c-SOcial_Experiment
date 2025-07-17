#include <atencionCPU.h>


void *atenderCPU(void *socketPtr) {
    int socket_cpu = *(int*)socketPtr;
    
    log_debug(logger, "Hilo atenderCPU creado, atendiendo socket %d", socket_cpu);
    free (socketPtr);
    
    t_paquete *respuesta = crear_paquete(SOYMEMORIA);
    agregar_a_paquete(respuesta, &nivelesTablas, sizeof(int));
    agregar_a_paquete(respuesta, &maximoEntradasTabla, sizeof(int));
    agregar_a_paquete(respuesta, &tamañoMarcos, sizeof(int));
    enviar_paquete(respuesta, socket_cpu);
    log_debug(logger, "Paquete enviado (pointer = %p)", respuesta);
    eliminar_paquete(respuesta);
    
    while(1) { // si se pierde la conexion, entra al if de abajo y termina el hilo

    int codigo_operacion = -42;
    t_list *pedido = recibir_paquete_lista(socket_cpu, MSG_WAITALL, &codigo_operacion); // problema //
    
    if(pedido == NULL) {
        log_debug(logger, "Se desconecto la CPU");
        liberarConexion(socket_cpu);
        pthread_exit(NULL);
    }
    

    switch(codigo_operacion) // Segun la operacion que me pida:
    {
    case PETICION_INSTRUCCION_MEMORIA:
        {
            int *pid = (int*)list_get(pedido, 1);
            int *pc = (int*)list_get(pedido, 3);
            log_debug(logger, "Fetch de proceso %d instruccion #%d", *pid, *pc);
            char pid_str[10];
            sprintf(pid_str, "%d", *pid);// transforma el numero de pid en un string

            if (!obtenerInstruccionesPorPID(*pid) || *pc >= cuantasInstruccionesDelPID(*pid)) {
                log_error(logger, "PID %d no encontrado o PC %d fuera de rango", *pid, *pc);
                pthread_exit(NULL);
            }

            aumentarMetricaInstruccinoesSolicitadas(*pid);
            
            char* instruccion = leerInstruccion(*pid, *pc);
            log_info(logger, "## PID: %d - Obtener instrucción: %d - Instrucción: %s", *pid, *pc, instruccion); 
            // Supongo que la instruccion ya tiene los parametros metidos
                // Armar respuesta y mandarla
            t_paquete *respuesta_cpu = crear_paquete(RESPUESTA_INSTRUCCION_MEMORIA);
            agregar_a_paquete(respuesta_cpu, instruccion, strlen(instruccion) + 1);
            // Se simula un retraso?
            simularRetrasoUnSoloNivel(); // TODO: Ver si esto va aca o no
            enviar_paquete(respuesta_cpu, socket_cpu);
            eliminar_paquete(respuesta_cpu);
            break;
            
        }
        case PETICION_MARCO_MEMORIA:
        {   
            int *pid = (int*)list_get(pedido, 1);
            log_debug(logger, "Se pide traducir direccion del proceso %d", *pid);
            int cantidadDeEntradas = (*(int *)list_get(pedido, 2)) / sizeof(int);
            simularRetrasoMultinivel();
            aumentarMetricaAccesoATablaDePaginasPorNiveles(*pid);
            int *entradasPaquete = (int *)list_get(pedido, 3);
            t_list *entradas = list_create();
            for(int i = 0; i < cantidadDeEntradas; i++) {
                list_add(entradas, &(entradasPaquete[i])); // Mientras exista el vector EntradasPaquete (y el pedido), va a existir los elementos de esta lista
            }
            int marco = obtenerMarcoDePaginaConPIDYEntradas(*pid, entradas);
            if(marco != -1) {
                //int dirMarco = direccionFisicaMarco(marco); // Devuelve el Offset, no el puntero en si
                // Habria que contemplar mandarle a la CPU un puntero en lugar de un entero que indique el offset
                // Mandar un puntero emularia mejor lo que realmente pasa, aunque si hay que reallocar la memoriaDeUsuario
                // se nos complicaría, ademas de que se siente como un crimen de guerra mandarle a otra computadora
                // un puntero al que no puede acceder, ya que direcciona a una ubicacion de otra pc.
                // Por el momento esto manda el offset. Se puede usar la funcion punteroAMarco en lugar de direccionFisicaMarco para cambiarlo
                t_paquete *respuesta_cpu = crear_paquete(RESPUESTA_MEMORIA_A_CPU_PETICION_MARCO);
                agregar_a_paquete(respuesta_cpu, &marco, sizeof(int));
                enviar_paquete(respuesta_cpu, socket_cpu);
                eliminar_paquete(respuesta_cpu);
            }
            else {
                log_debug(logger, "CPU de socket %d intentó acceder a una pagina sin marco asignado", socket_cpu);
                if(!esPaginaValida(*pid, entradas)) {
                    log_error(logger, "CPU de socket %d intentó acceder a una pagina por fuera de su tamaño. Posible error en el pseudocodigo", socket_cpu);
                    t_paquete *respuesta_cpu = crear_paquete(RESPUESTA_MEMORIA_A_CPU_PAGINA_NO_VALIDA);
                    enviar_paquete(respuesta_cpu, socket_cpu);
                    eliminar_paquete(respuesta_cpu);
                    break;
                }
                int marco = asignarSiguienteMarcoLibreDadasLasEntradas(*pid, entradas);
                log_debug(logger, "Se asigno el marco %d al PID %d", marco, *pid);
                //int dirMarco = direccionFisicaMarco(marco); // Devuelve el Offset, no el puntero en si
                t_paquete *respuesta_cpu = crear_paquete(RESPUESTA_MEMORIA_A_CPU_PETICION_MARCO);
                agregar_a_paquete(respuesta_cpu, &marco, sizeof(int));
                enviar_paquete(respuesta_cpu, socket_cpu);
                eliminar_paquete(respuesta_cpu);
            }
            list_destroy(entradas);
            break;
        }
        case PETICION_ESCRIBIR_EN_MEMORIA:
        {                               
            int *pid = (int*)list_get(pedido, 1); //
            aumentarMetricaEscrituraDeMemoria(*pid);

            int *direccion_fisica = (int*)list_get(pedido, 3); //
            char *datos = (char*)list_get(pedido, 5); //

            log_info(logger, "## PID: %d - Escritura - Dir. Física: %d - Tamaño: %d", *pid, *direccion_fisica, tamañoMarcos); //

            if (!es_valida_dir_fisica(pid, direccion_fisica, &tamañoMarcos)) {
                log_error(logger, "Dirección inválida: PID %d, dirección %d", *pid, *direccion_fisica);
                break;
            }

            pthread_mutex_lock(&MUTEX_MemoriaDeUsuario);
            memcpy(memoriaDeUsuario + *direccion_fisica, datos, tamañoMarcos);
            pthread_mutex_unlock(&MUTEX_MemoriaDeUsuario);

            t_paquete *respuesta = crear_paquete(RESPUESTA_ESCRIBIR_EN_MEMORIA);
            enviar_paquete(respuesta, socket_cpu);
            eliminar_paquete(respuesta);

            break;
        }
        case PETICION_LEER_DE_MEMORIA:
        {
            int *pid = (int*)list_get(pedido, 1); //
            aumentarMetricaLecturaDeMemoria(*pid);

            int *direccion_fisica = (int*)list_get(pedido, 3); //

            log_info(logger, "## PID: %d - Lectura - Dir. Física: %d - Tamaño: %d", *pid, *direccion_fisica, tamañoMarcos);
            
            if (!es_valida_dir_fisica(pid, direccion_fisica, &tamañoMarcos)) {
                log_error(logger, "Dirección inválida: PID %d, dirección %d", *pid, *direccion_fisica);
                break;
            }

            char *buffer = malloc(tamañoMarcos);

            pthread_mutex_lock(&MUTEX_MemoriaDeUsuario);
            memcpy(buffer, memoriaDeUsuario + *direccion_fisica, tamañoMarcos);
            pthread_mutex_unlock(&MUTEX_MemoriaDeUsuario);
            log_debug(logger, "A memoria le respondo con %s", buffer);

            t_paquete *respuesta = crear_paquete(RESPUESTA_LEER_DE_MEMORIA);
            agregar_a_paquete(respuesta, buffer, tamañoMarcos);
            enviar_paquete(respuesta, socket_cpu);
            eliminar_paquete(respuesta);

            free(buffer);
            break;

        }
        case PETICION_ESCRIBIR_EN_MEMORIA_LIMITADO:
        {                               
            int *pid = (int*)list_get(pedido, 1); //
            aumentarMetricaEscrituraDeMemoria(*pid);

            int *direccion_fisica = (int*)list_get(pedido, 3); //
            char *datos = (char*)list_get(pedido, 5); //
            int * sizeEscritura = (int*)list_get(pedido, 7);

            log_info(logger, "## PID: %d - Escritura - Dir. Física: %d - Tamaño: %d", *pid, *direccion_fisica, *sizeEscritura); //

            if (!es_valida_dir_fisica(pid, direccion_fisica, sizeEscritura)) {
                log_error(logger, "Dirección inválida: PID %d, dirección %d", *pid, *direccion_fisica);
                break;
            }

            pthread_mutex_lock(&MUTEX_MemoriaDeUsuario);
            memcpy(memoriaDeUsuario + *direccion_fisica, datos, *sizeEscritura);
            pthread_mutex_unlock(&MUTEX_MemoriaDeUsuario);

            t_paquete *respuesta = crear_paquete(RESPUESTA_ESCRIBIR_EN_MEMORIA);
            enviar_paquete(respuesta, socket_cpu);
            eliminar_paquete(respuesta);

            break;
        }
        case PETICION_LEER_DE_MEMORIA_LIMITADO:
        {
            int *pid = (int*)list_get(pedido, 1); //
            aumentarMetricaLecturaDeMemoria(*pid);

            int *direccion_fisica = (int*)list_get(pedido, 3); //
            int * sizeLectura = (int*)list_get(pedido, 5);

            log_info(logger, "## PID: %d - Lectura - Dir. Física: %d - Tamaño: %d", *pid, *direccion_fisica, *sizeLectura);

            if (!es_valida_dir_fisica(pid, direccion_fisica, &tamañoMarcos)) {
                log_error(logger, "Dirección inválida: PID %d, dirección %d", *pid, *direccion_fisica);
                break;
            }

            char *buffer = malloc(*sizeLectura);

            pthread_mutex_lock(&MUTEX_MemoriaDeUsuario);
            memcpy(buffer, memoriaDeUsuario + *direccion_fisica, *sizeLectura);
            pthread_mutex_unlock(&MUTEX_MemoriaDeUsuario);

            t_paquete *respuesta = crear_paquete(RESPUESTA_LEER_DE_MEMORIA);
            agregar_a_paquete(respuesta, buffer, *sizeLectura);
            enviar_paquete(respuesta, socket_cpu);
            eliminar_paquete(respuesta);

            free(buffer);
            break;

        }
        default:
        {
            // Error: una instruccion desconocida //
            log_error(logger, "Instruccion desconocida: código de operación %d", codigo_operacion);
            break;
        }
    }
    eliminar_paquete_lista(pedido);
    }
    pthread_exit(NULL);
}