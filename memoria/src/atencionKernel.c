#include <atencionKernel.h>

void * atenderKernel(void * socket){

    {
        t_paquete * respuesta = crear_paquete(SOYMEMORIA);
        enviar_paquete(respuesta, *(int*)socket);
        eliminar_paquete(respuesta);
    }

    CODIGO_OP codOp;
    t_list * pedido = recibir_paquete_lista(socket, MSG_WAITALL, codOp);
    if (pedido == NULL)
    {
        pthread_exit(NULL);
    }


    switch (codOp)
    {
    case SOLICITUD_MEMORIA_DUMP_MEMORY:
        bool dumpExitoso = true;

        int PID = list_get(pedido, 1);

        //TODO: DUMP MEMORY

        t_paquete * respuesta;
        if (dumpExitoso)
        respuesta = crear_paquete(RESPUESTA_DUMP_COMPLETADO);
        else 
        respuesta = crear_paquete(RESPUESTA_DUMP_ERROR);
        enviar_paquete(respuesta,*(int*)socket);
        eliminar_paquete(respuesta);
        break;
    case SOLICITUD_MEMORIA_CARGA_SWAP:
        bool cargaRealizada = true;

        int PID = list_get(pedido, 1);

        // TODO: cargar de swap a memoria

        t_paquete * respuesta;
        if (cargaRealizada)
        respuesta = crear_paquete(RESPUESTA_MEMORIA_PROCESO_CARGADO);
        else 
        respuesta = crear_paquete(RESPUESTA_MEMORIA_NO_HAY_MEMORIA_SUFICIENTE);
        enviar_paquete(respuesta,*(int*)socket);
        eliminar_paquete(respuesta);
        break;
    case SOLICITUD_MEMORIA_NUEVO_PROCESO:
        bool cargaRealizada = true;

        int PID = list_get(pedido, 1);
        int PATH = list_get(pedido, 3);
        int TAMAÑO = list_get(pedido, 5);

        // TODO: cargar de archivo pseudocodigo a memoria
        
        t_paquete * respuesta;
        if (cargaRealizada)
        respuesta = crear_paquete(RESPUESTA_MEMORIA_PROCESO_CARGADO);
        else 
        respuesta = crear_paquete(RESPUESTA_MEMORIA_NO_HAY_MEMORIA_SUFICIENTE);
        enviar_paquete(respuesta,*(int*)socket);
        eliminar_paquete(respuesta);
        break;
    case PROCESO_FINALIZADO_LIBERAR_MEMORIA:

        int PID = list_get(pedido, 1);

        // TODO: liberar la memoria
        
        t_paquete * respuesta;
        respuesta = crear_paquete(RESPUESTA_MEMORIA_LIBERADA_EXITOSAMENTE);
        enviar_paquete(respuesta,*(int*)socket);
        eliminar_paquete(respuesta);
        break;
    case PROCESO_SUSPENDIDO_ENVIAR_A_SWAP:

        int PID = list_get(pedido, 1);

        // TODO: pasar de memoria a swap
        
        t_paquete * respuesta;
        respuesta = crear_paquete(RESPUESTA_MEMORIA_PROCESO_ENVIADO_A_SWAP);
        enviar_paquete(respuesta,*(int*)socket);
        eliminar_paquete(respuesta);
        break;
    default:
        // ERROR
        break;
    }
    eliminar_paquete_lista(pedido);
}

