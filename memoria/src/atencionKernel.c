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

int realizarDump(int PID){
    log_debug(logger, "realizarDump de proceso %d", PID);
    t_list * marcos = marcosDelPid(PID);
    int qMarcos = list_size(marcos);
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
    if (fpArchivoDump == NULL){
        log_error(logger, "No se pudo realizar el dump del proceso %d.", PID);
        return 1;
    }
    pthread_mutex_lock(&MUTEX_MemoriaDeUsuario);
    char * buffer = malloc(tamañoMarcos * qMarcos + 1);
    for (int i = 0; i<qMarcos; i++){
    memcpy(buffer+i*tamañoMarcos, punteroAMarcoPorNumeroDeMarco(*(int*)list_get(marcos, i)), tamañoMarcos);
    }
    char * contenido = mem_hexstring(buffer , tamañoMarcos*qMarcos); // El hexString tira warnings de Valgrind. No lo puedo resolver sin tocar la libreria.
    fwrite(
        contenido,
        sizeof(char),
        strlen(contenido),
        fpArchivoDump);
    free(contenido);
    pthread_mutex_unlock(&MUTEX_MemoriaDeUsuario);
    list_destroy_and_destroy_elements(marcos, free);
    fclose(fpArchivoDump);
    free(pidAsString);
    free(buffer);
    free(archivoDump);
    return 0;
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
    //log_debug(logger, "Paquete recibido (socket = %d, pointer = %p, codOp = %d)", *(int*)socketPtr, pedido, codOp);
    // Recibo un paquete con el pedido

    if (pedido == NULL)
    {
        // Si se cerro la conexion, finalizar el hilo
        log_error(logger, "Se desconecto el kernel sin pedir nada");
        liberarConexion(socket);
        pthread_exit(NULL);
    }


    switch (codOp) // Segun la operacion que me pida:
    {
    case SOLICITUD_MEMORIA_DUMP_MEMORY:
        PID = list_get(pedido, 1);
        log_info(logger, "## PID: %d, Memory Dump solicitado", *PID);
        error = realizarDump(*PID);
        if (!error)
        respuesta = crear_paquete(RESPUESTA_DUMP_COMPLETADO);
        else 
        respuesta = crear_paquete(RESPUESTA_DUMP_ERROR);
        enviar_paquete(respuesta,socket);
        eliminar_paquete(respuesta);
        break;
    case SOLICITUD_MEMORIA_CARGA_SWAP: // Des suspensión del proceso en SWAP
        PID = list_get(pedido, 1);
        log_debug(logger, "Tamaño de proceso que se quiere dessuspender = %d, %d marcos", tamañoProceso(*PID), cantidadDePaginasDelProceso(*PID));
        log_debug(logger, "Espacio disponible: %d marcos", marcosDisponibles());
        if (!hayEspacio(tamañoProceso(*PID))) {
           error = 1;
        } else {
            log_debug(logger, "## Se saca %d de SWAP", *PID);
            if(dessuspenderProceso(*PID)){
            setEnMemoria(*PID);
            simularRetrasoSWAP();
            } else {error = 1;}

        }

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
        log_debug(logger, "Tamaño de proceso que se quiere dessuspender = %d, %d marcos", *TAMAÑO, cantidadDeMarcosParaAlmacenar(*TAMAÑO));
        log_debug(logger, "Espacio disponible: %d marcos", marcosDisponibles());
        if(hayEspacio(*TAMAÑO)){
        agregarProcesoATabla(*PID, *TAMAÑO);
        log_info(logger, "## PID: %d - Proceso Creado - Tamaño: %d", *PID, *TAMAÑO);
        cargarInstrucciones(*PID, PATH, *TAMAÑO);
        log_debug(logger, "Se cargan las instrucciones de (%d) provenientes de %s", *PID, PATH);
        aumentarMetricaSubidasAMemoriaPrincipal(*PID); // Subida inicial a memoria
        }else{
            error = 1;
        }
        log_debug(logger, "Tras operacion: %d marcos disponibles. Se añadio? = %d", marcosDisponibles(), !error);
        if (!error)
        respuesta = crear_paquete(RESPUESTA_MEMORIA_PROCESO_CARGADO);
        else 
        respuesta = crear_paquete(RESPUESTA_MEMORIA_NO_HAY_MEMORIA_SUFICIENTE);
        enviar_paquete(respuesta,socket);
        eliminar_paquete(respuesta);
        break;
    case PROCESO_FINALIZADO_LIBERAR_MEMORIA:
        
        PID = list_get(pedido, 1);
        Metricas m = getMetricasPorPID(*PID);
        log_info(logger, 
        "## PID: %d - Proceso Destruido - Métricas - Acc.T.Pag: %d; Inst.Sol.: %d; SWAP: %d; Mem.Prin.: %d; Lec.Mem.: %d; Esc.Mem.: %d", 
         *PID, m.accesosATP, m.instruccionesSolicitadas, m.bajadasASwap, m.subidasAMP, m.lecturasDeMemoria, m.escriturasDeMemoria);
        eliminarProcesoDeTabla(*PID);

        respuesta = crear_paquete(RESPUESTA_MEMORIA_LIBERADA_EXITOSAMENTE);
        enviar_paquete(respuesta,socket);
        eliminar_paquete(respuesta);
        break;
    case PROCESO_SUSPENDIDO_ENVIAR_A_SWAP: // suspensión del proceso en SWAP

        PID = list_get(pedido, 1);
        log_debug(logger, "## Se envia %d a SWAP", *PID);
        suspenderProceso(*PID);
        setEnSwap(*PID);
        simularRetrasoSWAP();
        // No deberia poder tirar error, solo si se acaba el espacio de disco(yo hice como que esto no es posible)
        
        respuesta = crear_paquete(RESPUESTA_MEMORIA_PROCESO_ENVIADO_A_SWAP);
        enviar_paquete(respuesta,socket);
        eliminar_paquete(respuesta);
        break;
    case VERIFICARCONEXION:
        log_debug(logger, "Kernel hizo un handshake pero no pidio hacer nada");
        break;
    default:
        // ERROR, se hizo una peticion que no correspondia 
        // (o no esta implementada en un case)
        break;
    }
    eliminar_paquete_lista(pedido);
    close(socket);
    pthread_exit(NULL);
}
