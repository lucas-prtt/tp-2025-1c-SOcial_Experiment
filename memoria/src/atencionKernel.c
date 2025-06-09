#include <atencionKernel.h>

/////////////////////////////////////////////
//               ATENCION                  //
/////////////////////////////////////////////

// Deje puesto todos los casos de peticiones posibles. 
// No todas importan para la entrega 2, 
// para poder probarlo seria solo cargar y eliminar procesos de memoria, es decir...
// case:  SOLICITUD_MEMORIA_NUEVO_PROCESO
// case: PROCESO_FINALIZADO_LIBERAR_MEMORIA 
// y default (tirar un error)

void realizarDump(int PID, char * contenidoDump){
    char * timestamp = timestampNow();
    char * pidAsString = malloc(20);
    sprintf(pidAsString, "%d", PID);
    char * archivoDump = malloc(strlen(directorioDump) + strlen(pidAsString) + 1 + strlen(timestamp) + 4 + 1 );
    strcpy(archivoDump, directorioDump);
    strcat(archivoDump, pidAsString);
    strcat(archivoDump, "-");
    strcat(archivoDump, timestamp);
    strcat(archivoDump, ".dmp");
    FILE * fpArchivoDump = fopen(archivoDump, "wb");
    fwrite(contenidoDump, sizeof(char), strlen(contenidoDump), fpArchivoDump);
    // TODO: SLEEP
    fclose(fpArchivoDump);
    free(pidAsString);
    free(timestamp);
    free(archivoDump);
}


void * atenderKernel(void * socketPtr){
    int socket = *(int*)socketPtr;
    log_debug(logger, "Hilo atenderKernel creado, atendiendo socket %d", socket);
    free (socketPtr); // Libero el socket del heap, lo guardé en el stack

    int * PID;
    char * PATH;
    int * TAMAÑO;
    int error = 0; // Actua como booleano, para utilizar en el futuro
                   // al implementar la logica de las peticiones del kernel

    t_paquete * respuesta = crear_paquete(SOYMEMORIA);
    enviar_paquete(respuesta, socket);
    log_debug(logger, "Paquete enviado (pointer = %p)", respuesta);
    eliminar_paquete(respuesta);
    // Envio un paquete que completa el handshake

    int codOp=-42; // Nunca deberia tomar este valor. Si loguea codOp = -42, es que no asigno codOp al recibir el paquete
    t_list * pedido = recibir_paquete_lista(socket, MSG_WAITALL, &codOp);
    log_debug(logger, "Paquete recibido (socket = %d, pointer = %p, codOp = %d)", *(int*)socketPtr, pedido, codOp);
    // Recibo un paquete con el pedido

    if (pedido == NULL)
    {
        // Si se cerro la conexion, finalizar el hilo
        liberarConexion(socket);
        pthread_exit(NULL);
    }


    switch (codOp) // Segun la operacion que me pida:
    {
    case SOLICITUD_MEMORIA_DUMP_MEMORY:
        PID = list_get(pedido, 1);
        log_debug(logger, "## PID: %d, Memory Dump solicitado", *PID);

        //TODO: DUMP MEMORY
        // (modifica el valor de error)

        if (!error)
        respuesta = crear_paquete(RESPUESTA_DUMP_COMPLETADO);
        else 
        respuesta = crear_paquete(RESPUESTA_DUMP_ERROR);
        enviar_paquete(respuesta,socket);
        eliminar_paquete(respuesta);
        break;
    case SOLICITUD_MEMORIA_CARGA_SWAP:

        PID = list_get(pedido, 1);

        // TODO: cargar de swap a memoria
        // (modifica el valor de error)

        if (!error)
        respuesta = crear_paquete(RESPUESTA_MEMORIA_PROCESO_CARGADO);
        else 
        respuesta = crear_paquete(RESPUESTA_MEMORIA_NO_HAY_MEMORIA_SUFICIENTE);
        enviar_paquete(respuesta,socket);
        eliminar_paquete(respuesta);
        break;
    case SOLICITUD_MEMORIA_NUEVO_PROCESO:

        PID = list_get(pedido, 1);
        PATH = list_get(pedido, 3);
        TAMAÑO = list_get(pedido, 5);
        
        if(hayEspacio(*TAMAÑO)){
        agregarProcesoATabla(*PID, *TAMAÑO);
        log_info(logger, "## PID: %d - Proceso Creado - Tamaño: %d", *PID, *TAMAÑO);
        cargarInstrucciones(*PID, PATH, *TAMAÑO);
        }else{
            error = 1;
        }
        if (!error)
        respuesta = crear_paquete(RESPUESTA_MEMORIA_PROCESO_CARGADO);
        else 
        respuesta = crear_paquete(RESPUESTA_MEMORIA_NO_HAY_MEMORIA_SUFICIENTE);
        enviar_paquete(respuesta,socket);
        eliminar_paquete(respuesta);
        break;
    case PROCESO_FINALIZADO_LIBERAR_MEMORIA:

        PID = list_get(pedido, 1);

        eliminarProcesoDeTabla(*PID);

        respuesta = crear_paquete(RESPUESTA_MEMORIA_LIBERADA_EXITOSAMENTE);
        enviar_paquete(respuesta,socket);
        eliminar_paquete(respuesta);
        break;
    case PROCESO_SUSPENDIDO_ENVIAR_A_SWAP:

        PID = list_get(pedido, 1);

        // TODO: pasar de memoria a swap
        // No deberia poder tirar error, solo si se acaba el espacio de disco
        
        respuesta = crear_paquete(RESPUESTA_MEMORIA_PROCESO_ENVIADO_A_SWAP);
        enviar_paquete(respuesta,socket);
        eliminar_paquete(respuesta);
        break;
    default:
        // ERROR, se hizo una peticion que no correspondia 
        // (o no esta implementada en un case)
        break;
    }
    eliminar_paquete_lista(pedido);
    pthread_exit(NULL);
}

