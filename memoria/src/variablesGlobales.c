#include "variablesGlobales.h"


t_list * tablaDeProcesos = NULL;
int maximoEntradasTabla;
int nivelesTablas;
int tamañoMarcos;
int tamañoMemoriaDeUsuario;
int * PIDPorMarco; // Vectpr:  PIDPorMarco[numeroDeMarco] = PID o -1 (vacio)
int numeroDeMarcos;
void * memoriaDeUsuario;

pthread_mutex_t MUTEX_tablaDeProcesos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MUTEX_PIDPorMarco = PTHREAD_MUTEX_INITIALIZER;


// @brief Se debe ejecutar antes de utilizar cualquier funcion o variable del archivo variablesGlobales.h
void inicializarVariablesGlobales(int sizeTabla, int qNiveles, int sizeMemoria, int SizeMarcos){
    tablaDeProcesos = list_create();
    maximoEntradasTabla = sizeTabla;
    nivelesTablas = qNiveles;
    tamañoMarcos = SizeMarcos;
    tamañoMemoriaDeUsuario = sizeMemoria;
    memoriaDeUsuario = malloc(sizeMemoria);
    numeroDeMarcos = tamañoMemoriaDeUsuario / tamañoMarcos;
    PIDPorMarco = malloc(numeroDeMarcos);
    for (int i = 0; i<numeroDeMarcos; i++){
        PIDPorMarco[i] = -1;
    }
    
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
            return *raiz;
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
// @brief Elimina un proceso de la tabla y libera los recursos asociados al proceso. No actualiza el PIDporMarco
void eliminarProcesoDeTabla(int PIDEliminado){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * elemento = removerInfoProcesoConPID(PIDEliminado);
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    if (elemento->instrucciones) {
        list_destroy_and_destroy_elements(elemento->instrucciones, free);
    }
    liberarArbolDePaginas(elemento->TP);
    //TODO: Actualizar PIDporMarco
    free(elemento);
}
// @brief Dado un PID y una lista de entradas asociadas a una pagina, obtiene el marco
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
void * punteroAMarco(int numeroDeMarco){
    return memoriaDeUsuario + numeroDeMarco * tamañoMarcos;
}
//@brief Devuelve la division entre tamaño y tamañoMarcos, redondeada para arriba
int cantidadDeMarcosParaAlmacenar(int tamaño){
    return (tamaño+tamañoMarcos-1)/tamañoMarcos;
}
//@brief Devuelve la cantidad de marcos ocupados en memoria, usando el maximo de marcos ocupables por proceso
int marcosOcupados(){
    int acum = 0;
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    int tam_lista_procesos = list_size(tablaDeProcesos); 
    for (int i=0; i<tam_lista_procesos; i++){
        acum +=  cantidadDeMarcosParaAlmacenar(*((int*)list_get(tablaDeProcesos, i)));
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
    t_list * inst = obtenerInfoProcesoConPID(PID)->instrucciones;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return inst;
}

