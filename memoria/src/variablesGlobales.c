#include "variablesGlobales.h"


t_list * tablaDeProcesos = NULL;
int maximoEntradasTabla;
int nivelesTablas;
int tamañoMarcos;
int tamañoMemoriaDeUsuario;
int * PIDPorMarco; 
int numeroDeMarcos;
void * memoriaDeUsuario;
char * directorioPseudocodigo;
char * directorioDump;
char * directorioSwap;
int retrasoAcceso;
int retrasoSWAP; // Falta implementarlo
t_list* tablaSwap = NULL;
t_list* espaciosLibresSwapentrePaginas = NULL;
int paginasLibresTotalesSwapEntreProcesos = 0;

pthread_mutex_t MUTEX_tablaDeProcesos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MUTEX_PIDPorMarco = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MUTEX_MemoriaDeUsuario = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MUTEX_SiguienteMarcoLibre = PTHREAD_MUTEX_INITIALIZER; // Mutex para que el valor de siguienteMarcoLibre() no quede desactualizado
// @brief Se debe ejecutar antes de utilizar cualquier funcion o variable del archivo variablesGlobales.h
void inicializarVariablesGlobales(int sizeTabla, int qNiveles, int sizeMemoria, int SizeMarcos, char * PathPseudocodigo, char * PathDUMP, int retAcc, int retSWAP, char * PathSwap){
    log_debug(logger, "Inicializando variables");
    tablaDeProcesos = list_create();
    maximoEntradasTabla = sizeTabla;
    directorioPseudocodigo = PathPseudocodigo;
    directorioDump = PathDUMP;
    nivelesTablas = qNiveles;
    tamañoMarcos = SizeMarcos;
    directorioSwap = PathSwap;
    tamañoMemoriaDeUsuario = sizeMemoria;
    log_debug(logger, "Inicializacion antes de memoria de usuario");
    memoriaDeUsuario = malloc(sizeMemoria);
    memset(memoriaDeUsuario, 0, sizeMemoria);
    numeroDeMarcos = tamañoMemoriaDeUsuario / tamañoMarcos;
    //log_debug(logger, "Inicializacion antes de tabla de marcos");
    PIDPorMarco = malloc(numeroDeMarcos * sizeof(int));
    //log_debug(logger, "Luego de malloc de PIDPorMarco");
    retrasoAcceso = retAcc;
    retrasoSWAP = retSWAP;
    //log_debug(logger, "Antes del ciclo PIDPORMARCO");
    for (int i = 0; i<numeroDeMarcos; i++){
        PIDPorMarco[i] = -1;
    }
    //Tabla donde van a estar que procesos estan en SWAP
    tablaSwap = list_create();
    //Tabla de los espacios libres del Swap
    espaciosLibresSwapentrePaginas = list_create();
}


//////////////////// ARBOL INICIO ////////////////////
// @brief Crea un nivel de la tabla de pagias y rellena todas sus entradas con NULL
void ** crearNivelTablaDePaginas(int maximoEntradasTabla){
    void ** tabla = malloc(sizeof(void*)*maximoEntradasTabla);
    for(int i=0; i<maximoEntradasTabla; i++)
        tabla[i] = NULL;
    return tabla;
}
    void * buscarOCrearAux(void ** raiz, t_list * entradas, int niveles, int nivelActual, int maximoEntradasTabla)
    {   
        if (nivelActual == niveles+1)
            return raiz;
        int indice = *(int*)list_get(entradas, nivelActual-1);
        void ** siguienteTabla = raiz[indice];
        if (siguienteTabla == NULL)
        {
            if(nivelActual == niveles){
                raiz[indice] = malloc(sizeof(int));
                *(int*)(raiz[indice]) = -1 ;
            }
            else{
                raiz[indice] = crearNivelTablaDePaginas(maximoEntradasTabla);
            }
        }
        return buscarOCrearAux(raiz[indice], entradas, niveles, nivelActual+1, maximoEntradasTabla);
    }
// @brief  Busca el marco del arbol dada las entradas y si no lo encuentra lo crea y lo deja en -1
int * buscarOCrear(void ** arbolDePaginas, t_list * entradas, int niveles, int maximoEntradasTabla)
{
    return (int *) buscarOCrearAux(arbolDePaginas, entradas, niveles, 1, maximoEntradasTabla);

}
// @brief Recorre el arbol de paginas segun las entradas y sobreescribe el marco actual por numeroMarco
void asignarMarcoAPagina(int numeroMarco, void ** arbolDePaginas, t_list * entradas)
{
    int * ubicacion = buscarOCrear (arbolDePaginas, entradas, nivelesTablas, maximoEntradasTabla);
    log_debug(logger, "asignarMarcoAPagina devuelve ubicacion = %p, contenido = %d", ubicacion, *ubicacion);
    *ubicacion = numeroMarco;
}
// @brief Wrapper para buscarOCrear => Busca un marco de pagina en un arbol y si no esta lo crea y deja en -1
int leerMarcoDePagina(void ** arbolDePaginas, t_list * entradas)
{
    int * ubicacion = buscarOCrear (arbolDePaginas, entradas, nivelesTablas, maximoEntradasTabla);
    return * ubicacion;
}
void liberarArbolDePaginasAux(void ** arbolDePaginas, int nivelesRestantes){
    if (arbolDePaginas == NULL){
        return;
    }
    if(nivelesRestantes == 1){
        for(int i=0; i<maximoEntradasTabla; i++){
            if(arbolDePaginas[i] != NULL){
                free(arbolDePaginas[i]);
            }
        }
    }else{
        for(int i=0; i<maximoEntradasTabla; i++)
            liberarArbolDePaginasAux((void**)(arbolDePaginas[i]), nivelesRestantes-1);
    }   
    free(arbolDePaginas);
    return;
}
// @brief Elimina un arbol de paginas y todas sus subpaginas y marcos. Debe estar bien configurado nivelTablas
void liberarArbolDePaginas(void ** arbolDePaginas){
    liberarArbolDePaginasAux(arbolDePaginas, nivelesTablas);
}

//////////////////// ARBOL FIN //////////////////


/////////////////// TP INCIO /////////////////

// @brief Agrega el proceso a la tabla, deja las instrucciones en NULL
void agregarProcesoATabla(int nuevoPID, int tamañoProceso)
{ 
    PIDInfo * nuevoElemento = malloc(sizeof(PIDInfo));
    nuevoElemento->PID = nuevoPID;
    nuevoElemento->TP = crearNivelTablaDePaginas(maximoEntradasTabla);
    nuevoElemento->stats.accesosATP = 0;
    nuevoElemento->stats.instruccionesSolicitadas = 0;
    nuevoElemento->stats.bajadasASwap = 0;
    nuevoElemento->stats.subidasAMP = 0;
    nuevoElemento->stats.lecturasDeMemoria = 0;
    nuevoElemento->stats.escriturasDeMemoria = 0;
    nuevoElemento->TamMaxProceso = tamañoProceso;
    nuevoElemento->instrucciones = NULL;
    nuevoElemento->enSwap = 0;
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    list_add(tablaDeProcesos, nuevoElemento);
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    
}
// @brief Obtiene el infoProceso de un proceso dado el PID. No es thread safe: Usar mutex
PIDInfo * obtenerInfoProcesoConPID(int PIDBuscado){
    #ifndef __INTELLISENSE__
    bool coincide (void * elemento)
    {
        return ((PIDInfo*)elemento)->PID == PIDBuscado;
    }
    return (PIDInfo*)list_find(tablaDeProcesos, coincide);
    #endif
}
// @brief  Obtiene el infoProceso de un proceso dado el PID y lo saca de la lista de tablaDeProcesos. No es thread safe: Usar mutex
PIDInfo * removerInfoProcesoConPID(int PIDBuscado){
    #ifndef __INTELLISENSE__
    bool coincide (void * elemento)
    {
        return ((PIDInfo*)elemento)->PID == PIDBuscado;
    }
    return (PIDInfo*)list_remove_by_condition(tablaDeProcesos, coincide);
    #endif
}
// @brief En la tabla PIDPorMarco, marca todos los marcos con el PID que coincide con -1
void eliminarProcesoDePIDPorMarco(int PID){
    pthread_mutex_lock(&MUTEX_PIDPorMarco);
    for(int i = 0; i<numeroDeMarcos; i++)
    {
        if(PIDPorMarco[i] == PID)
            PIDPorMarco[i] = -1;
    }
    pthread_mutex_unlock(&MUTEX_PIDPorMarco);
}
// @brief Elimina el arbol de paginas del PID pero lo deja inicializado para poder buscar y crear en el mismo a futuro (Desde afuera, se verá como si todos los marcos fueran -1)
void vaciarTablaDePaginasDePID(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * proceso = obtenerInfoProcesoConPID(PID);
    liberarArbolDePaginas(proceso->TP);
    proceso->TP = crearNivelTablaDePaginas(maximoEntradasTabla);
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
}
// @brief Elimina un proceso de la tabla y libera los recursos asociados al proceso. No actualiza el PIDporMarco
void eliminarProcesoDeTabla(int PIDEliminado){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * elemento = removerInfoProcesoConPID(PIDEliminado);
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    if (elemento->instrucciones) {
        list_destroy_and_destroy_elements(elemento->instrucciones, free);
    }
    liberarArbolDePaginas(elemento->TP);
    eliminarProcesoDePIDPorMarco(PIDEliminado);
    free(elemento);
}

// @brief Dado un PID y una lista de entradas asociadas a una pagina, obtiene el marco.
int obtenerMarcoDePaginaConPIDYEntradas(int PID, t_list * entradas){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    int r = leerMarcoDePagina(obtenerInfoProcesoConPID(PID)->TP, entradas);
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return r;
}
// @brief asigna el marco a un proceso mediante el PID, las entradas y el valor a asignar
void asignarMarcoAPaginaConPIDyEntradas(int PID, t_list * entradas, int marco){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    asignarMarcoAPagina(marco, obtenerInfoProcesoConPID(PID)->TP, entradas);
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    pthread_mutex_lock(&MUTEX_PIDPorMarco);
    PIDPorMarco[marco] = PID;
    pthread_mutex_unlock(&MUTEX_PIDPorMarco);
    return; 
}
// @brief actualiza el marco de PIDPorMarco, seteandolo en -1
void removerPaginaDeMarco(int marco)
{   
    //No afecta el arbol del proceso
    pthread_mutex_lock(&MUTEX_PIDPorMarco);
    PIDPorMarco[marco] = -1;
    pthread_mutex_unlock(&MUTEX_PIDPorMarco);
    return; 
}
// @brief Devuelve el PID del marco "numeroDeMarco". Utiliza mutex para que sea thread safe
int PIDDelMarco(int numeroDeMarco){
    pthread_mutex_lock(&MUTEX_PIDPorMarco);
    int r = PIDPorMarco[numeroDeMarco];
    pthread_mutex_unlock(&MUTEX_PIDPorMarco);
    return r;
}
/////////////////// TP FIN /////////////////////

////////////// MANEJO DE MARCOS ///////////////


//@brief Devuelve el puntero a un marco dentro del void * memoriaDeUsuario. Cuando se use, solo escribir hasta la posicion + tamañoMarcos
void * punteroAMarcoPorNumeroDeMarco(int numeroDeMarco){
    return memoriaDeUsuario + numeroDeMarco * tamañoMarcos;
}

//@brief Dado un numero de marco, devuelve el offset que representa la direccion fisica del marco en nuestra "memoria". Para acceder, a esto hay que agregarle el puntero a la memoria fisica
int direccionFisicaMarco(int numeroDeMarco){
    return numeroDeMarco * tamañoMarcos;
}

//@brief Devuelve el puntero a marco dada la "direccion fisica"
void * punteroAMarcoPorDireccionFisica(int direccionFisica){
    return memoriaDeUsuario + direccionFisica;
}

//@brief Devuelve la division entre tamaño y tamañoMarcos, redondeada para arriba
int cantidadDeMarcosParaAlmacenar(int tamaño){
    return (tamaño+tamañoMarcos-1)/tamañoMarcos;
}
//@brief Devuelve la cantidad de marcos ocupados en memoria, usando el maximo de marcos ocupables por proceso

int estaCargado(PIDInfo * Proceso){
    return !Proceso->enSwap;
}
//OJO que la funcion esta mal porque retorna antes de liberar el mutex, si se quiere descomentar y usar arreglar primero.
//int estaCargadoPid(int PID){
//    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
//    PIDInfo * Proceso = obtenerInfoProcesoConPID(PID);
//    return !Proceso->enSwap;
//    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
//}
void setEnSwap(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * Proceso = obtenerInfoProcesoConPID(PID);
    Proceso->enSwap = 1;    
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);

}
void setEnMemoria(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * Proceso = obtenerInfoProcesoConPID(PID);
    Proceso->enSwap = 0;    
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
}
int marcosOcupados(){
    int acum = 0;
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    int tam_lista_procesos = list_size(tablaDeProcesos); 
    for (int i=0; i<tam_lista_procesos; i++){
        if(estaCargado((PIDInfo*)list_get(tablaDeProcesos, i)))
        acum +=  cantidadDeMarcosParaAlmacenar((((PIDInfo*)list_get(tablaDeProcesos, i))->TamMaxProceso));
    }
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return acum;
}

//@brief Determina la cantidad de marcos disponibles, teniendo en cuenta el maxmo de marcos ocupables por los procesos
int marcosDisponibles(){
    return numeroDeMarcos - marcosOcupados();
}
//@brief Determina si hay suficiente espacio para un proceso. tamañoRequerido en bytes, no marcos
bool hayEspacio(int tamañoRequerido){
    return (marcosDisponibles() >= (cantidadDeMarcosParaAlmacenar(tamañoRequerido)));
}
//@brief stats.accesosATP++; (con MUTEX, tomando solo el PID)
void aumentarMetricaAccesoATablaDePaginas(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * el = obtenerInfoProcesoConPID(PID);
    el->stats.accesosATP++;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return;
}
void aumentarMetricaAccesoATablaDePaginasPorNiveles(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * el = obtenerInfoProcesoConPID(PID);
    el->stats.accesosATP+= nivelesTablas;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return;
}

//@brief stats.bajadasASwap++; (con MUTEX, tomando solo el PID)
void aumentarMetricaBajadasASwap(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * el = obtenerInfoProcesoConPID(PID);
    el->stats.bajadasASwap++;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return;
}
//@brief stats.escriturasDeMemoria++; (con MUTEX, tomando solo el PID)
void aumentarMetricaEscrituraDeMemoria(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * el = obtenerInfoProcesoConPID(PID);
    el->stats.escriturasDeMemoria++;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return;
}
//@brief stats.instruccionesSolicitadas++; (con MUTEX, tomando solo el PID)
void aumentarMetricaInstruccinoesSolicitadas(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * el = obtenerInfoProcesoConPID(PID);
    el->stats.instruccionesSolicitadas++;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return;
}
//@brief stats.lecturasDeMemoria++; (con MUTEX, tomando solo el PID)
void aumentarMetricaLecturaDeMemoria(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * el = obtenerInfoProcesoConPID(PID);
    el->stats.lecturasDeMemoria++;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return;
}
//@brief stats.subidasAMP++; (con MUTEX, tomando solo el PID)
void aumentarMetricaSubidasAMemoriaPrincipal(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * el = obtenerInfoProcesoConPID(PID);
    el->stats.subidasAMP++;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return;
}
//@brief Devuelve Metricas (no el puntero a metricas) de un proceso, usando mutex
Metricas getMetricasPorPID(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    Metricas r = ((PIDInfo*)obtenerInfoProcesoConPID(PID))->stats;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return r;
}
//@brief Agrega lista de instrucciones a un proceso, mediante un PID. Usa MUTEX. No se debe liberar la lista luego
void agregarInstruccionesAPID(int PID, t_list * instruccionesNuevas){ // No liberar lista de instrucciones
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * proceso = obtenerInfoProcesoConPID(PID);
    proceso->instrucciones = instruccionesNuevas;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
}   
//@brief obtiene lista de instrucciones del proceso: No es una copia: No liberar ni modificar innecesariamente
t_list * obtenerInstruccionesPorPID(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    log_trace(logger, "Se van a obtener las instrucciones de %d", PID);
    t_list * inst = obtenerInfoProcesoConPID(PID)->instrucciones;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return inst;
}


t_list * marcosDelPid(int PID){
    t_list * marcos = list_create();
    pthread_mutex_lock(&MUTEX_PIDPorMarco);
    int * marco;
    for(int i=0; i<numeroDeMarcos; i++)
    {
        if(PIDPorMarco[i] == PID){
            marco = malloc(sizeof(marco));
            *marco = i;
            list_add(marcos, marco);
        }
    }
    pthread_mutex_unlock(&MUTEX_PIDPorMarco);
    return marcos;
}

void simularRetrasoMultinivel(){
    usleep(retrasoAcceso*nivelesTablas*1000); // En milisegundos, no microsegundos
}
void simularRetrasoUnSoloNivel(){
    usleep(retrasoAcceso*1000);
}
void simularRetrasoSWAP(){
    usleep(retrasoSWAP*1000);
}

int siguienteMarcoLibre(){
    pthread_mutex_lock(&MUTEX_PIDPorMarco);
    for(int i=0; i<numeroDeMarcos; i++){
        if(PIDPorMarco[i] == -1){
        pthread_mutex_unlock(&MUTEX_PIDPorMarco);
        return i;
        }
    }
    pthread_mutex_unlock(&MUTEX_PIDPorMarco);
    return -1;
}
int asignarSiguienteMarcoLibreDadasLasEntradas(int PID, t_list * entradas){
    pthread_mutex_lock(&MUTEX_SiguienteMarcoLibre);
    int sig = siguienteMarcoLibre();
    if(sig != -1)
    asignarMarcoAPaginaConPIDyEntradas(PID, entradas, siguienteMarcoLibre());
    else
    log_error(logger, "Se intentó reservar mas espacio de la cuenta");
    pthread_mutex_unlock(&MUTEX_SiguienteMarcoLibre);
    return sig;
}

int convertirEntradasAPagina(t_list * entradas){
    int qentradas = list_size(entradas);
    int acum = 0;
    for(int i=0; i<qentradas; i++){
        acum += (*(int*)list_get(entradas, i)) *  (int)pow(maximoEntradasTabla, qentradas-i-1);
    }
    return acum;
}
int cantidadDePaginasDelProceso(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * proceso = obtenerInfoProcesoConPID(PID);
    int qmarcos = cantidadDeMarcosParaAlmacenar(proceso->TamMaxProceso);
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return qmarcos;
}

int esPaginaValida(int PID, t_list * entradas){
    return 
    cantidadDePaginasDelProceso(PID) // N, (>=1)
      >   
    convertirEntradasAPagina(entradas) ; // de 0 a N-1 esta bien
}

int tamañoProceso(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * proceso = obtenerInfoProcesoConPID(PID);
    int temp = proceso->TamMaxProceso;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return temp;
}