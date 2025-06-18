#include <atencionCPU.h>


void *atenderCPU(void *socketPtr) {
    int socket_cpu = *(int*)socketPtr;
    
    log_debug(logger, "Hilo atenderCPU creado, atendiendo socket %d", socket_cpu);
    free (socketPtr);
    
    t_paquete *respuesta = crear_paquete(SOYCPU);
    enviar_paquete(respuesta, socket_cpu);
    log_debug(logger, "Paquete enviado (pointer = %p)", respuesta); // que hace esto?
    // Respuesta: Esto era para comprobar que el paquete existia cuando escribí atencionKernel. Realmente se puede sacar
    eliminar_paquete(respuesta);
    
    // TODO: Envolver todo esto dentro de un while
    
    int codigo_operacion = -42;
    t_list *pedido = recibir_paquete_lista(socket_cpu, MSG_WAITALL, &codigo_operacion);
    log_debug(logger, "Paquete recibido (socket = %d, pointer = %p, codigo_operacion = %d)", *(int*)socketPtr, pedido, codigo_operacion);
    
    if(pedido == NULL) {
        liberarConexion(socket_cpu);
        pthread_exit(NULL);
    }
    

    switch(codigo_operacion) // Segun la operacion que me pida:
    {
    case PETICION_INSTRUCCION_MEMORIA:
        {
            int *pid = (int*)list_get(pedido, 1);
            int *pc = (int*)list_get(pedido, 3);

            char pid_str[10];
            sprintf(pid_str, "%d", *pid);

            if (!obtenerInstruccionesPorPID(*pid) || *pc >= cuantasInstruccionesDelPID(*pid)) {
                log_error(logger, "PID %d no encontrado o PC %d fuera de rango", *pid, *pc);
                pthread_exit(NULL);
            }

            char* instruccion = leerInstruccion(*pid, *pc);

                // Armar respuesta y mandarla
            t_paquete *respuesta_cpu = crear_paquete(RESPUESTA_INSTRUCCION_MEMORIA);
            agregar_a_paquete(respuesta_cpu, instruccion, strlen(instruccion) + 1);
            enviar_paquete(respuesta_cpu, socket_cpu);
            eliminar_paquete(respuesta_cpu);
            break;
            
        }case PETICION_TRADUCCION_DIRECCION:
        {   
            simularRetrasoMultinivel();
            int *pid = (int*)list_get(pedido, 1);
            int cantidadDeEntradas = (*(int*)list_get(pedido, 2))/sizeof(int);
            int *entradasPaquete = (int*)list_get(pedido, 3);
            t_list * entradas = list_create();
            for (int i = 0; i < cantidadDeEntradas; i++){
                list_add(entradas, &(entradasPaquete[i])); // Mientras exista el vector EntradasPaquete (y el pedido), va a existir los elementos de esta lista
            }
            int marco = obtenerMarcoDePaginaConPIDYEntradas(*pid, entradas);
            list_destroy(entradas);
            if(marco != -1){
                int dirMarco = direccionFisicaMarco(marco); // Devuelve el Offset, no el puntero en si
                // Habria que contemplar mandarle a la CPU un puntero en lugar de un entero que indique el offset
                // Mandar un puntero emularia mejor lo que realmente pasa, aunque si hay que reallocar la memoriaDeUsuario
                // se nos complicaría, ademas de que se siente como un crimen de guerra mandarle a otra computadora
                // un puntero al que no puede acceder, ya que direcciona a una ubicacion de otra pc.
                // Por el momento esto manda el offset. Se puede usar la funcion punteroAMarco en lugar de direccionFisicaMarco para cambiarlo
                t_paquete *respuesta_cpu = crear_paquete(RESPUESTA_MEMORIA_A_CPU_DIRECCION_PAGINA);
                agregar_a_paquete(respuesta_cpu, &dirMarco, sizeof(int));
                enviar_paquete(respuesta_cpu, socket_cpu);
                eliminar_paquete(respuesta_cpu);
            }else{
                log_debug(logger, "CPU de socket %d intentó acceder a una pagina sin marco asignado", socket_cpu);
                if(!esPaginaValida(*pid, entradas)){
                    log_error(logger, "CPU de socket %d intentó acceder a una pagina por fuera de su tamaño. Posible error en el pseudocodigo", socket_cpu);
                    t_paquete *respuesta_cpu = crear_paquete(RESPUESTA_MEMORIA_A_CPU_PAGINA_NO_VALIDA);
                    enviar_paquete(respuesta_cpu, socket_cpu);
                    eliminar_paquete(respuesta_cpu);
                    break;
                }
                int marco = asignarSiguienteMarcoLibreDadasLasEntradas(*pid, entradas);
                log_debug(logger, "Se asigno el marco %d al PID %d", marco, *pid);
                int dirMarco = direccionFisicaMarco(marco); // Devuelve el Offset, no el puntero en si
                t_paquete *respuesta_cpu = crear_paquete(RESPUESTA_MEMORIA_A_CPU_DIRECCION_PAGINA);
                agregar_a_paquete(respuesta_cpu, &dirMarco, sizeof(int));
                enviar_paquete(respuesta_cpu, socket_cpu);
                eliminar_paquete(respuesta_cpu);
            }
            break;
        }
        case PETICION_ESCRIBIR_EN_MEMORIA:
        {                                               //Revisar que tira errores de compilacion (prueben usar el comando make para ver los errores que tira)
            int *pid = (int*)list_get(pedido, 1); //
            int *direccion_fisica = (int*)list_get(pedido, 3); //
            char *datos = (char*)list_get(pedido, 5); //
            int tamanio = strlen(datos) + 1;

            log_info(logger, "## PID: %d - Escritura - Dir. Física: %d - Tamaño: <TAMAÑO>", *pid, *direccion_fisica); // Dice tamaño... será datos?

            if (!es_valida_dir_fisica(pid, direccion_fisica, tamanio)) {
                // error
            } else {
                memcpy(memoriaDeUsuario + *direccion_fisica, datos, tamanio);
                t_paquete *respuesta_peticion_escribir = crear_paquete(RESPUESTA_ESCRIBIR_EN_MEMORIA);
                enviar_paquete(respuesta_peticion_escribir, socket_cpu);
                eliminar_paquete(respuesta_peticion_escribir);
            }
            break;
        }
        case PETICION_LEER_DE_MEMORIA:
        {
            int *pid = (int*)list_get(pedido, 1); //
            int *direccion_fisica = (int*)list_get(pedido, 3); //
            int *tamanio = (int*)list_get(pedido, 5); //

            log_info(logger, "## PID: %d - Lectura - Dir. Física: %d - Tamaño: %d", *pid, *direccion_fisica, *tamanio);

            if (!es_valida_dir_fisica(pid, direccion_fisica, tamanio)) {
                // enviar error
            } else {
                // leer bytes desde el buffer de memoria
                void* datos = malloc(tamanio);
                memcpy(datos, memoriaDeUsuario + *direccion_fisica, tamanio);
                //
                t_paquete *respuesta_peticion_leer = crear_paquete(RESPUESTA_LEER_DE_MEMORIA);
                agregar_a_paquete(respuesta_peticion_leer, datos, tamanio);
                enviar_paquete(respuesta_peticion_leer, socket_cpu);
                eliminar_paquete(respuesta_peticion_leer);
                free(datos);
            }
            break;
        }
        default:
        {
            // Error: una instruccion desconocida //
            break;
        }
    }
    eliminar_paquete_lista(pedido);
    pthread_exit(NULL);
}