#include <atencionKernel.h>

void * atenderKernel(void * socketPtr){
    int socket = *(int*)socketPtr;
    int * PID;
    char * PATH;
    int * TAMAÑO;

    int error = 0;

    t_paquete * respuesta = crear_paquete(SOYMEMORIA);
    enviar_paquete(respuesta, socket);
    eliminar_paquete(respuesta);

    int codOp;
    t_list * pedido = recibir_paquete_lista(socket, MSG_WAITALL, &codOp);
    if (pedido == NULL)
    {
        pthread_exit(NULL);
    }


    switch (codOp)
    {
    case SOLICITUD_MEMORIA_DUMP_MEMORY:
        PID = list_get(pedido, 1);
        log_debug(logger, "## PID: %d, Memory Dump solicitado", *PID);

        //TODO: DUMP MEMORY

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

        log_info(logger, "## PID: %d - Proceso Creado - Tamaño: %d", *PID, *TAMAÑO);

        // TODO: cargar de archivo pseudocodigo a memoria
        
        if (!error)
        respuesta = crear_paquete(RESPUESTA_MEMORIA_PROCESO_CARGADO);
        else 
        respuesta = crear_paquete(RESPUESTA_MEMORIA_NO_HAY_MEMORIA_SUFICIENTE);
        enviar_paquete(respuesta,socket);
        eliminar_paquete(respuesta);
        break;
    case PROCESO_FINALIZADO_LIBERAR_MEMORIA:

        PID = list_get(pedido, 1);

        // TODO: liberar la memoria
        
        respuesta = crear_paquete(RESPUESTA_MEMORIA_LIBERADA_EXITOSAMENTE);
        enviar_paquete(respuesta,socket);
        eliminar_paquete(respuesta);
        break;
    case PROCESO_SUSPENDIDO_ENVIAR_A_SWAP:

        PID = list_get(pedido, 1);

        // TODO: pasar de memoria a swap
        
        respuesta = crear_paquete(RESPUESTA_MEMORIA_PROCESO_ENVIADO_A_SWAP);
        enviar_paquete(respuesta,socket);
        eliminar_paquete(respuesta);
        break;
    default:
        // ERROR
        break;
    }
    eliminar_paquete_lista(pedido);
    pthread_exit(NULL);
}

