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

int * buscarOCrear(void ** arbolDePaginas, t_list * entradas, int niveles, int maximoEntradasTabla)
{
    return (int *) buscarOCrearAux(arbolDePaginas, entradas, niveles, 1, maximoEntradasTabla);

}

void asignarMarcoAPagina(int numeroMarco, void ** arbolDePaginas, t_list * entradas)
{
    int * ubicacion = buscarOCrear (arbolDePaginas, entradas, nivelesTablas, maximoEntradasTabla);
    *ubicacion = numeroMarco;
}
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

void liberarArbolDePaginas(void ** arbolDePaginas){
    liberarArbolDePaginasAux(arbolDePaginas, nivelesTablas);
}

//////////////////// ARBOL FIN //////////////////


/////////////////// TP INCIO /////////////////

void agregarProcesoATabla(int nuevoPID, int tamañoProceso){
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
PIDInfo * obtenerInfoProcesoConPID(int PIDBuscado){
    #ifndef __INTELLISENSE__
    bool coincide (void * elemento)
    {
        return ((PIDInfo*)elemento)->PID == PIDBuscado;
    }
    return (PIDInfo*)list_find(tablaDeProcesos, coincide);
    #endif
}
PIDInfo * removerInfoProcesoConPID(int PIDBuscado){
    #ifndef __INTELLISENSE__
    bool coincide (void * elemento)
    {
        return ((PIDInfo*)elemento)->PID == PIDBuscado;
    }
    return (PIDInfo*)list_remove_by_condition(tablaDeProcesos, coincide);
    #endif
}

void eliminarProcesoDeTabla(int PIDEliminado){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * elemento = removerInfoProcesoConPID(PIDEliminado);
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    if (elemento->instrucciones) {
        list_destroy_and_destroy_elements(elemento->instrucciones, free);
    }
    liberarArbolDePaginas(elemento->TP);
    free(elemento);
}
int obtenerMarcoDePaginaConPIDYEntradas(int PID, t_list * entradas){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    int r = leerMarcoDePagina(obtenerInfoProcesoConPID(PID)->TP, entradas);
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return r;
}
void asignarMarcoAPaginaConPIDyEntradas(int PID, t_list * entradas, int marco){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    asignarMarcoAPagina(marco, obtenerInfoProcesoConPID(PID)->TP, entradas);
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    pthread_mutex_lock(&MUTEX_PIDPorMarco);
    PIDPorMarco[marco] = PID;
    pthread_mutex_unlock(&MUTEX_PIDPorMarco);
    return; 
}
void removerPaginaDeMarco(int marco)
{   
    //No afecta el arbol del proceso
    pthread_mutex_lock(&MUTEX_PIDPorMarco);
    PIDPorMarco[marco] = -1;
    pthread_mutex_unlock(&MUTEX_PIDPorMarco);
    return; 
}

int PIDDelMarco(int numeroDeMarco){
    pthread_mutex_lock(&MUTEX_PIDPorMarco);
    int r = PIDPorMarco[numeroDeMarco];
    pthread_mutex_unlock(&MUTEX_PIDPorMarco);
    return r;
}
/////////////////// TP FIN /////////////////////

////////////// MANEJO DE MARCOS ///////////////

void * punteroAMarco(int numeroDeMarco){
    return memoriaDeUsuario + numeroDeMarco * tamañoMarcos;
}

int cantidadDeMarcosParaAlmacenar(int tamaño){
    return (tamaño+tamañoMarcos-1)/tamañoMarcos;
}
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

int marcosDisponibles(){
    return numeroDeMarcos - marcosOcupados();
}

bool hayEspacio(int tamañoRequerido){
    return (marcosDisponibles() >= (cantidadDeMarcosParaAlmacenar(tamañoRequerido)));
}

void aumentarMetricaAccesoATablaDePaginas(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * el = obtenerInfoProcesoConPID(PID);
    el->stats.accesosATP++;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return;
}

void aumentarMetricaBajadasASwap(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * el = obtenerInfoProcesoConPID(PID);
    el->stats.bajadasASwap++;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return;
}

void aumentarMetricaEscrituraDeMemoria(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * el = obtenerInfoProcesoConPID(PID);
    el->stats.escriturasDeMemoria++;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return;
}
void aumentarMetricaInstruccinoesSolicitadas(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * el = obtenerInfoProcesoConPID(PID);
    el->stats.instruccionesSolicitadas++;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return;
}
void aumentarMetricaLecturaDeMemoria(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * el = obtenerInfoProcesoConPID(PID);
    el->stats.lecturasDeMemoria++;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return;
}
void aumentarMetricaSubidasAMemoriaPrincipal(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * el = obtenerInfoProcesoConPID(PID);
    el->stats.subidasAMP++;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return;
}

Metricas getMetricasPorPID(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    Metricas r = ((PIDInfo*)obtenerInfoProcesoConPID(PID))->stats;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return r;
}

void agregarInstruccionesAPID(int PID, t_list * instruccionesNuevas){ // No liberar lista de instrucciones
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    PIDInfo * proceso = obtenerInfoProcesoConPID(PID);
    proceso->instrucciones = instruccionesNuevas;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
}   

t_list * obtenerInstruccionesPorPID(int PID){
    pthread_mutex_lock(&MUTEX_tablaDeProcesos);
    t_list * inst = obtenerInfoProcesoConPID(PID)->instrucciones;
    pthread_mutex_unlock(&MUTEX_tablaDeProcesos);
    return inst;
}

