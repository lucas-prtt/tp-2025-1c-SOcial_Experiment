#include "utils_memory.h"
extern pthread_mutex_t MUTEX_PIDPorMarco;
t_list* lista_instrucciones = NULL;

ModulosConectados conexiones;
t_dictionary* instrucciones_por_pid;

int crearSocketConfig(t_config* config, char opcion[]){
    int puertoConfig = config_get_int_value(config, opcion);
    char puerto[7];
    sprintf(puerto, "%d", puertoConfig);
    return crearSocketServer(puerto);
}

void* aceptarConexiones(void* socketPtr) {

    // Guardo el socket de conexion inicialen el stack
    int socket = *(int*)socketPtr; 
    
    // Libero el socket repetido en HEAP
    free(socketPtr);
    
    while (1) { // Hasta que se cierre el thread:
        struct sockaddr_in dirCliente;
        socklen_t tamDireccion = sizeof(dirCliente);

        // Acepto una conexion entrante y la guardo en socketCliente
        int socketCliente = accept(socket, (void*)&dirCliente, &tamDireccion);
        if (socketCliente == -1) {
            // Si accept da error
            log_error(logger, "Error al aceptar conexión");
            // Continue hace que termine el ciclo de while sin ejecutar
            // todo lo que sigue y se ejecute otro ciclo a continuacion
            // (Equivalente a un else que envuelva toda la logica posterior
            //  hasta el fin del while)
            continue;
        }

        // Esto no se porque quisieron hacerlo. 
        // Muestra la IP y el Puerto
        char ipCliente[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &dirCliente.sin_addr, ipCliente, INET_ADDRSTRLEN);
        int puertoCliente = ntohs(dirCliente.sin_port);
        log_debug(logger, "Conexión entrante desde %s: %d", ipCliente, puertoCliente);

        // Creo un hilo de atencion y le mando el socketCliente generado en este hilo
        pthread_t hiloCliente;
        int* socketClientePtr = malloc(sizeof(int));
        *socketClientePtr = socketCliente;

        pthread_create(&hiloCliente, NULL, atenderConexion, socketClientePtr);
        pthread_detach(hiloCliente);

    }
    return NULL;
}

void* atenderConexion(void* socketPtr) {
    log_debug(logger, "Hilo de conexion creado para socket %d", *(int*)socketPtr);
    // Cada vez que se hace un conectarSocketClient(ipMemoria, puertoMemoria) 
    // desde otro modulo, se ejecuta esta funcion en un nuevo thread
    
    int id; // El id va a ser el codOp del paquete 
            // (del enum idModulo para el handshake)
    
    t_list * paqueteRecibido = recibir_paquete_lista(*(int*)socketPtr, MSG_WAITALL, &id);
    log_debug(logger, "Paquete recibido: codOp = %d, pointer paquete = %p", id, paqueteRecibido);
    
    // Se recibe un paquete con un codOp SOYKERNEL (0) o SOYCPU (1). (y una lista vacia)

    if (paqueteRecibido == NULL) {
        // Si en vez de lista vacia no tengo la lista 
        // (NULL en lugar de lista de 0 elementos creada con list_create())
        // Entonces: 
        // - No destruyo la lista, pues no existe
        // - Cierro la conexion al socket
        // - Termino con el thread
        log_error(logger, "Se cerró la conexión");
        close(*(int*)socketPtr);
        return NULL;
    }
    // Si el paquete si se recibio...
    log_debug(logger, "El paquete no es nulo");

    // El contenido esta vacio (lista vacia) por lo que
    // la eliminamos para guardar espacio (el ID mantiene su valor)
    list_destroy(paqueteRecibido);

    switch (id) {
        case SOYKERNEL:
            // Si el ID del paquete indica que es el kernel:
            log_info(logger, "## Kernel Conectado - FD del socket: %d", *(int*)socketPtr);
            pthread_t hiloKernel;
            // Creo un thread para atender al kernel y le mando el socket de la conexion
            pthread_create(&hiloKernel, NULL, atenderKernel, socketPtr);
            pthread_detach(hiloKernel);
            break;
        case SOYCPU:
            // Si el ID del paquete indica que es una CPU
            log_debug(logger, "## CPU Conectado - FD del socket: %d", *(int*)socketPtr);
            pthread_t hiloCPU;
            // Creo un thread para atender la CPU y le mando el socket de la conexion
            pthread_create(&hiloCPU, NULL, atenderCPU, socketPtr);
            pthread_detach(hiloCPU);
            break;
        default:
            // Si el ID no es ni de CPU ni de Kernel:
            // Hubo algun error
            log_error(logger, "Modulo conectado desconocido. Fallo el Handshake.");
            // No tiene sentido mantener el socket abierto si hubo error, asi que lo cierro
            close(*(int*)socket);
            return NULL;
    }
    // El hilo se cierra tras el handshake que indica el comportamiento necesario
    return NULL; 
}

// busca las instrucciones por PID
t_list* obtener_instrucciones_por_pid(uint32_t pid){
    for (int i = 0; i < list_size(lista_instrucciones); i++)
    {
        t_instrucciones_por_pid* entrada = list_get(lista_instrucciones, i);
        if (entrada->pid == pid){
            return entrada->instrucciones;
        }
    }
    return NULL;
    
}

int obtener_espacio_libre() {
    return 128; // Simulación de 128 páginas libres
    // Considerar el uso de la funcion marcosDisponibles() en variablesGlobales.c
}

bool es_valida_dir_fisica(int* pid, int* direccion_fisica, int* tamanio) {
    int inicio = *direccion_fisica;
    int fin = inicio + *tamanio;

    if (inicio < 0 || fin > tamañoMemoriaDeUsuario) {
        return false;
    }

    // Validar que todas las pags que se pisan esten dentro del rango de marco
    int pagina_inicio = inicio / tamañoMarcos;
    int pagina_fin = (fin - 1) / tamañoMarcos;


    // Falta que se verifique que lo marcos escrito corresponden al proceso

    if (pagina_inicio < 0 || pagina_fin >= numeroDeMarcos) {
        return false;
    }
    
    return true;
}

bool asignarPaginaAlProceso (int pid, int numPagina) {
    int marcoLibre = siguienteMarcoLibre();
    if (marcoLibre == -1) {
        log_error(logger, "No hay marcos libres para asignar a PID %d, página %d", pid, numPagina);
        return false;
    }

    t_list * entradas = entradasDesdeNumeroDePagina(numPagina);

    asignarMarcoAPaginaConPIDyEntradas(pid, entradas, marcoLibre);

    pthread_mutex_lock(&MUTEX_PIDPorMarco);
    PIDPorMarco[marcoLibre] = pid;
    pthread_mutex_unlock(&MUTEX_PIDPorMarco);

    log_debug(logger, "Asignado marco %d al PID %d (página %d)", marcoLibre, pid, numPagina);

    list_destroy_and_destroy_elements(entradas, free);
    return true;
}

void suspenderProceso(int pid){
    FILE* swap = fopen("swapfile.bin", "rb+");
    if (!swap) {
        log_error(logger, "No se pudo abrir swapfile.bin");
        return;
    }

    int cantidadPaginas = cantidadDePaginasDelProceso(pid);

    for (int nroPagina = 0; nroPagina < cantidadPaginas; nroPagina++) {
        // Obtener entradas del árbol a partir del número de página es decir el camino a seguir en el arbol en forma de lista
        t_list* entradas = entradasDesdeNumeroDePagina(nroPagina);

        // Buscar marco actual (si tiene asignado)
        int marco = obtenerMarcoDePaginaConPIDYEntradas(pid, entradas);

        if (marco == -1) {
            list_destroy(entradas);
            continue; // página no está en memoria, nada que guardar
        }

        // Leer contenido de la página
        void* contenido = malloc(tamañoMarcos);
        memcpy(contenido, punteroAMarcoPorNumeroDeMarco(marco), tamañoMarcos);//guardo el contenido de la pagina

        // Calcular offset dentro del swapfile
        int cantidadMaximaPaginasPorProceso = pow(maximoEntradasTabla, nivelesTablas);//esto se puede cambiar porque podria hacer de alguna forma que sea variable pero quedarian espacios y seria mucho más complejo
        int offset = (pid * cantidadMaximaPaginasPorProceso + nroPagina) * tamañoMarcos;

        fseek(swap, offset, SEEK_SET);
        fwrite(contenido, tamañoMarcos, 1, swap);

        // Guardar entrada en la tabla de swap
        EntradaSwap* entradaSwap = malloc(sizeof(EntradaSwap));
        entradaSwap->pid = pid;
        entradaSwap->nro_pagina = nroPagina;
        entradaSwap->offset = offset;
        list_add(tablaSwap, entradaSwap);

        free(contenido);
        list_destroy(entradas);
    }

    fclose(swap);

    // Liberar marcos y tabla de páginas
    eliminarProcesoDePIDPorMarco(pid);
    vaciarTablaDePaginasDePID(pid);

    aumentarMetricaBajadasASwap(pid);
}

//Sirve para a partir del nroPagina obtener el camino a seguir del arbol
t_list* entradasDesdeNumeroDePagina(int nroPagina) {
    t_list* entradas = list_create();

    for (int i = nivelesTablas - 1; i >= 0; i--) {
        int divisor = pow(maximoEntradasTabla, i);
        int entrada = nroPagina / divisor;
        list_add(entradas, int_de(entrada));
        nroPagina %= divisor;
    }

    return entradas;
}

int* int_de(int n) {
    int* p = malloc(sizeof(int));
    *p = n;
    return p;
}

void dessuspenderProceso(int pid) {
    FILE* swap = fopen("swapfile.bin", "rb");
    if (!swap) {
        log_error(logger, "No se pudo abrir swapfile.bin");
        return;
    }

    int cantidadPaginas = cantidadDePaginasDelProceso(pid);
    int cantidadMaximaPaginasPorProceso = pow(maximoEntradasTabla, nivelesTablas);

    for (int nroPagina = 0; nroPagina < cantidadPaginas; nroPagina++) {
        // Calcular offset exacto donde está la página del proceso en el swap
        int offset = (pid * cantidadMaximaPaginasPorProceso + nroPagina) * tamañoMarcos;

        // Leer la página desde el archivo swap
        void* buffer = malloc(tamañoMarcos);
        fseek(swap, offset, SEEK_SET);
        fread(buffer, tamañoMarcos, 1, swap);

        // Buscar un marco libre
        int marcoLibre = siguienteMarcoLibre();
        if (marcoLibre == -1) {
            log_error(logger, "No hay marcos disponibles para cargar página %d del PID %d", nroPagina, pid);
            free(buffer);
            continue;// No se que hacer si no hay espacio por que en teoria no deberia pasar por la memoria cuando tiene el espacio deberia avisar para que mande un proceso
        }

        // Copiar el contenido al marco
        memcpy(punteroAMarcoPorNumeroDeMarco(marcoLibre), buffer, tamañoMarcos);

        // Calcular las entradas del árbol de páginas para esta página
        t_list* entradas = entradasDesdeNumeroDePagina(nroPagina);

        // Asignar el marco a la página en la tabla de páginas del proceso
        asignarMarcoAPaginaConPIDyEntradas(pid, entradas, marcoLibre);// Esta funcion entiendo que hace eso pero tengo que preguntar todavia

        aumentarMetricaSubidasAMemoriaPrincipal(pid);

        list_destroy_and_destroy_elements(entradas, free);
        free(buffer);
    }

    fclose(swap);
}
