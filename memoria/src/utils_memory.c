#include "utils_memory.h"
extern pthread_mutex_t MUTEX_PIDPorMarco;
t_list* lista_instrucciones = NULL;
pthread_mutex_t mutex_lista_instrucciones = PTHREAD_MUTEX_INITIALIZER;

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
    pthread_mutex_lock(&mutex_lista_instrucciones); // No quiero que se agregen mas Procesos mientras se busca
    int a = list_size(lista_instrucciones);
    for (int i = 0; i < a; i++)
    {
        t_instrucciones_por_pid* entrada = list_get(lista_instrucciones, i);
        if (entrada->pid == pid){
            pthread_mutex_unlock(&mutex_lista_instrucciones);
            return entrada->instrucciones;
        }
    }
    pthread_mutex_unlock(&mutex_lista_instrucciones);

    return NULL;
    
}

bool es_valida_dir_fisica(int* pid, int* direccion_fisica, int* tamanio) {
    int inicio = *direccion_fisica;
    int fin = inicio + *tamanio;

    if (inicio < 0 || fin > tamañoMemoriaDeUsuario) {
        log_error(logger, "Se solicito escribir en una direccion fuera del espacio de la memoria");
        return false;
    }

    // Validar que todas las pags que se pisan esten dentro del rango de marco
    int marco_inicio = inicio / tamañoMarcos;
    int marco_fin = (fin - 1) / tamañoMarcos;

    if (marco_inicio < 0 || marco_fin >= numeroDeMarcos) {
        log_error(logger, "Se solicito escribir en un marco fuera del espacio de la memoria");
        // No deberia salir nunca este error, debido a la comprobacion anterior
        return false;
    }

    /*
    for(int i=marco_inicio; i<=marco_fin; i++){
        if(PIDdelMarco(i) != *pid){
        log_error(logger, "Se intento escribir en una direccion que pertenece a otro proceso");
        log_error(logger, "El marco %d es invalido, pertenece al proceso (%d)", i, PIDdelMarco(i));
        return false;}
    }
    */

    
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
    FILE* swap = abrirSwapFile();

    int cantidadPaginas = cantidadDePaginasDelProceso(pid);

    //tengo que ver si en realidad no hay ningun marco en memoria a pesar de que cantidad de paginas diga que hay 1 pero que no existe
    for (int nroPagina = 0; nroPagina <= cantidadPaginas; nroPagina++) {
        // Obtener entradas del árbol a partir del número de página es decir el camino a seguir en el arbol en forma de lista
        t_list* entradas = entradasDesdeNumeroDePagina(nroPagina);

        // Buscar marco actual (si tiene asignado)
        int marco = obtenerMarcoDePaginaConPIDYEntradas(pid, entradas);

        list_destroy_and_destroy_elements(entradas, free);
        if (marco == -1) {
            if (nroPagina == cantidadPaginas){//me fijo si es la ultima pagina del for
                //si es la ultima pagina y no esta el marco en memoria tampoco(osea ninguno lo esta) entonces solo se anota en la tabla de swap en el final de todo(para evitar compactacion) y se actualiza las metricas
                // Guardar entrada en la tabla de swap
                int offset = obtenerFinDeSwap();
                EntradaSwap* entradaSwap = malloc(sizeof(EntradaSwap));
                entradaSwap->pid = pid;
                entradaSwap->nro_pagina = (-1);//le pongo que sea la pagina -1 para cuando desuspendo saber que no tiene que copiar nada a memoria
                entradaSwap->offset = offset;
                //log_warning(logger,"pid:%d nro_pagina:%d offset:%d",pid,nroPagina,offset);
                list_add(tablaSwap, entradaSwap);

                fclose(swap);

                // Liberar marcos y tabla de páginas(no deberia ser necesario pero me da miedo que se rompa algo)
                eliminarProcesoDePIDPorMarco(pid);
                vaciarTablaDePaginasDePID(pid);

                aumentarMetricaBajadasASwap(pid);
                return;//termino suspenderProceso
                }//si no tiene, prueba la siguiente pagina
        }else break;//si encuenta que hay un marco ocupado sale del for
    }

// 1. Buscar primer espacio libre contiguo que alcance
    int offsetInicial = -1;
    EspacioLibre* hueco;
    for (int i = 0; i < list_size(espaciosLibresSwapentrePaginas); i++) {
        hueco = list_get(espaciosLibresSwapentrePaginas, i);
        if (hueco->paginasLibres >= cantidadPaginas) {
            offsetInicial = hueco->punto_incio;
            hueco->punto_incio += cantidadPaginas * tamañoMarcos;
            hueco->paginasLibres -= cantidadPaginas;
            if (hueco->paginasLibres == 0) list_remove_and_destroy_element(espaciosLibresSwapentrePaginas, i, free);
            paginasLibresTotalesSwapEntreProcesos -= cantidadPaginas;
            break;
        }
    }

    // 2. Si no hay hueco contiguo, compactar si hay espacio disperso
    if ((offsetInicial == -1)) {
        if (paginasLibresTotalesSwapEntreProcesos >= cantidadPaginas) {
            log_debug(logger, "Compactando SWAP para PID %d", pid);
            compactarSwap();
            offsetInicial = obtenerFinDeSwap();
        } else {
            log_debug(logger, "No hay suficiente espacio total en huecos para PID %d, escribiendo al final igual (puede dejar fragmentación)", pid);
            offsetInicial = obtenerFinDeSwap();
        }
    }
    int paginasNoEsta = 0;
    for (int nroPagina = 0; nroPagina < cantidadPaginas; nroPagina++) {
        // Obtener entradas del árbol a partir del número de página es decir el camino a seguir en el arbol en forma de lista
        t_list* entradas = entradasDesdeNumeroDePagina(nroPagina);

        // Buscar marco actual (si tiene asignado)
        int marco = obtenerMarcoDePaginaConPIDYEntradas(pid, entradas);
        if (marco == -1) {
            list_destroy_and_destroy_elements(entradas, free);
            paginasNoEsta++;
            continue;
        }
        // Leer contenido de la página
        void* contenido = malloc(tamañoMarcos);
        memcpy(contenido, punteroAMarcoPorNumeroDeMarco(marco), tamañoMarcos);//guardo el contenido de la pagina

        // Calcular offset dentro del swapfile
        int offset = offsetInicial + (nroPagina-paginasNoEsta) * tamañoMarcos;

        fseek(swap, offset, SEEK_SET);
        fwrite(contenido, tamañoMarcos, 1, swap);

        // Guardar entrada en la tabla de swap
        EntradaSwap* entradaSwap = malloc(sizeof(EntradaSwap));
        entradaSwap->pid = pid;
        entradaSwap->nro_pagina = nroPagina;
        entradaSwap->offset = offset;
        //log_warning(logger,"pid:%d nro_pagina:%d offset:%d",pid,nroPagina,offset);
        list_add(tablaSwap, entradaSwap);

        free(contenido);
        list_destroy_and_destroy_elements(entradas, free);
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

void compactarSwap() {
    FILE* swap = abrirSwapFile();
    
    if (list_size(tablaSwap) <= 1) {
        fclose(swap);
        return; // No hace falta compactar si hay 0 o 1 páginas
    }

    list_sort(tablaSwap, compararEntradasSwap); // ordena por offset

    int nuevoOffset = 0;
    EntradaSwap* entrada;
    for (int i = 0; i < list_size(tablaSwap); i++) {
        entrada = list_get(tablaSwap, i);

        if (entrada->offset != nuevoOffset) {
            void* buffer = malloc(tamañoMarcos);
            //copia el contenido de la pagina a reubicar al buffer
            fseek(swap, entrada->offset, SEEK_SET);
            fread(buffer, tamañoMarcos, 1, swap);

            //copia el contenido del buffer en la nueva posicion para eliminar el hueco
            fseek(swap, nuevoOffset, SEEK_SET);
            fwrite(buffer, tamañoMarcos, 1, swap);

            entrada->offset = nuevoOffset; // actualiza nuevo offset
            free(buffer);
        }

        nuevoOffset += tamañoMarcos;
    }

    // reconstruye espaciosLibres
    list_clean_and_destroy_elements(espaciosLibresSwapentrePaginas, free);

    fclose(swap);
}

bool compararEntradasSwap(void * a, void * b) {
    return (((EntradaSwap *) a)->offset <= ((EntradaSwap *) b)->offset);
}

int obtenerFinDeSwap() {
    int maxOffset = 0;
    for (int i = 0; i < list_size(tablaSwap); i++) {
        EntradaSwap* entrada = list_get(tablaSwap, i);
        int finPagina = 0;
        if(entrada->nro_pagina != -1){
        finPagina= entrada->offset + tamañoMarcos;}

        if (finPagina > maxOffset) {
            maxOffset = finPagina;
        }
    }
    return maxOffset;
}

int dessuspenderProceso(int pid) {
    FILE* swap = abrirSwapFile();

    // Entradas de swap del PID
    t_list* paginasDelProceso = list_create();

    // Buscar todas las entradas de tablaSwap que pertenezcan al PID
    for (int i = 0; i < list_size(tablaSwap); i++) {
        EntradaSwap* entrada = list_get(tablaSwap, i);
        if (entrada->pid == pid) {
            list_add(paginasDelProceso, entrada);
        }
    }

    // Si el proceso realmente no esta en el SWAP entonces hay un error
    if (list_size(paginasDelProceso) == 0) {
        log_error(logger, "No hay entradas en swap para el PID %d", pid);
        fclose(swap);
        list_destroy(paginasDelProceso);
        abort();
    }

    for (int i = 0; i < list_size(paginasDelProceso); i++) {
        EntradaSwap* entrada = list_get(paginasDelProceso, i);

        if(entrada->nro_pagina == (-1)){
            //solo lo saca de la taba swap y actualiza metricas
            aumentarMetricaSubidasAMemoriaPrincipal(pid);
            // Eliminar entrada de tablaSwap
            for (int i = 0; i < list_size(tablaSwap);i++) {
                EntradaSwap* entrada = list_get(tablaSwap, i);
                if (entrada->pid == pid) {
                    list_remove_and_destroy_element(tablaSwap, i, free);
                    break;//Salgo porque obligatoriamente va a ser 1
                    }
            }
            list_destroy(paginasDelProceso);
            fclose(swap);
            return 1;
        }

        // Leer la página desde el archivo swap
        void* buffer = malloc(tamañoMarcos);
        fseek(swap, entrada->offset, SEEK_SET);
        fread(buffer, tamañoMarcos, 1, swap);

        // Calcular las entradas del árbol de páginas para esta página
        t_list* entradas = entradasDesdeNumeroDePagina(entrada->nro_pagina);

        // Buscar un marco libre y asignar el marco a la página en la tabla de páginas del proceso
        int marcoLibre = asignarSiguienteMarcoLibreDadasLasEntradas(pid,entradas);
        if (marcoLibre == -1) {
            log_error(logger, "No hay marcos disponibles para cargar página %d del PID %d", entrada->nro_pagina, pid);
            free(buffer);
            return 0;
        }
        
        // Copiar el contenido al marco
        memcpy(punteroAMarcoPorNumeroDeMarco(marcoLibre), buffer, tamañoMarcos);
        //log_warning(logger,"marcoLibre:%d punteroAMarcoPorNumeroDeMarco(marcoLibre):%p",marcoLibre,punteroAMarcoPorNumeroDeMarco(marcoLibre));
        aumentarMetricaSubidasAMemoriaPrincipal(pid);

        list_destroy_and_destroy_elements(entradas, free);
        free(buffer);
    }

    // Eliminar entradas de tablaSwap y actualizar espacios libres
    liberarEspacioSwap(pid);

    list_destroy(paginasDelProceso);
    fclose(swap);
    return 1;
}

void liberarEspacioSwap(int pid) {
    int paginasLiberadas = 0;
    int inicioBloque = -1;

    for (int i = 0; i < list_size(tablaSwap); ) {
        EntradaSwap* entrada = list_get(tablaSwap, i);
        if (entrada->pid == pid) {
            if (inicioBloque == -1 || entrada->offset < inicioBloque) {
                inicioBloque = entrada->offset;
            }

            paginasLiberadas++;

            // Eliminar directamente y no incrementar el índice, porque se reacomodan
            list_remove_and_destroy_element(tablaSwap, i, free);
        } else {
            i++; // Solo avanzamos si no borramos
        }
    }
    //No existe el caso en el que no hay paginasLiberadas porque ubiera sido detectado antes cuando dije "Si el proceso realmente no esta en el SWAP entonces no hay nada que pasar a memoria"

    int finBloqueLiberado = inicioBloque + paginasLiberadas * tamañoMarcos;
    if (finBloqueLiberado < obtenerFinDeSwap()) {//me fijo que no sea el ultimo proceso en SWAP
    // Crear nuevo hueco
    EspacioLibre* nuevoEspacio = malloc(sizeof(EspacioLibre));
    nuevoEspacio->punto_incio = inicioBloque;
    nuevoEspacio->paginasLibres = paginasLiberadas;

    list_add(espaciosLibresSwapentrePaginas, nuevoEspacio);
    paginasLibresTotalesSwapEntreProcesos += paginasLiberadas;

    mergearEspaciosLibres(); // Función que junta huecos adyacentes
    }
}

void mergearEspaciosLibres() {
    // Ordenamos por punto de inicio
    list_sort(espaciosLibresSwapentrePaginas,compararEspaciosPorInicio);
    

    for (int i = 0; i < list_size(espaciosLibresSwapentrePaginas) - 1; ) {
        EspacioLibre* actual = list_get(espaciosLibresSwapentrePaginas, i);
        EspacioLibre* siguiente = list_get(espaciosLibresSwapentrePaginas, i + 1);

        int fin_actual = actual->punto_incio + actual->paginasLibres * tamañoMarcos;
        if (fin_actual == siguiente->punto_incio) {
            // Son adyacentes, fusionar
            actual->paginasLibres += siguiente->paginasLibres;
            list_remove_and_destroy_element(espaciosLibresSwapentrePaginas, i + 1, free);
            // No incrementamos i porque puede seguir habiendo adyacentes
        } else {
            i++; // Sólo avanzamos si no hubo merge
        }
    }
}

bool compararEspaciosPorInicio(void * a, void * b) {
    return (((EspacioLibre *) a)->punto_incio <= ((EspacioLibre *) b)->punto_incio);
}

FILE * abrirSwapFile(){
    FILE * swap;
    swap = fopen(directorioSwap, "r+");
    if(!swap){
        log_debug(logger, "No se pudo abrir el swapfile. Se va a crear uno nuevo");
        swap = fopen(directorioSwap, "w+");
        if(!swap){
            log_error(logger, "No se pudo abrir ni crear el swapfile. Compruebe que el directorio introducido en el archivo de config existe");
            abort();
        }
    }
    return swap;
}


void borrarSwapfile(){
    FILE * swap;
    swap = fopen(directorioSwap, "w");
    if(!swap){
        log_error(logger, "No se pudo crear un swapfile vacio");
    }
    fclose(swap);
    return;
}

