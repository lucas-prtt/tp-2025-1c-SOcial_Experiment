#include "variablesGlobales.h"


t_list * tablaDeProcesos = NULL;



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

void agregarProcesoATabla(int nuevoPID){
    PIDyTP * nuevoElemento = malloc(sizeof(PIDyTP));
    nuevoElemento->PID = nuevoPID;
    nuevoElemento->TP = crearNivelTablaDePaginas(maximoEntradasTabla);
    list_add(tablaDeProcesos, nuevoElemento);
    
}
PIDyTP * obtenerProcesoYTPConPID(int PIDBuscado){
    #ifndef __INTELLISENSE__
    bool coincide (void * elemento)
    {
        return ((PIDyTP*)elemento)->PID == PIDBuscado;
    }
    return (PIDyTP*)list_find(tablaDeProcesos, coincide);
    #endif
}
PIDyTP * removerProcesoYTPConPID(int PIDBuscado){
    #ifndef __INTELLISENSE__
    bool coincide (void * elemento)
    {
        return ((PIDyTP*)elemento)->PID == PIDBuscado;
    }
    return (PIDyTP*)list_remove_by_condition(tablaDeProcesos, coincide);
    #endif
}

void eliminarProcesoDeTabla(int PIDEliminado){
    PIDyTP * elemento = removerProcesoYTPConPID(PIDEliminado);
    liberarArbolDePaginas(elemento->TP);
    free(elemento);
}
int obtenerMarcoDePaginaConPIDYEntradas(int PID, t_list * entradas){
    return leerMarcoDePagina(obtenerProcesoYTPConPID(PID)->TP, entradas);
}
void asignarMarcoAPaginaConPIDyEntradas(int PID, t_list * entradas, int marco){
    asignarMarcoAPagina(marco, obtenerProcesoYTPConPID(PID)->TP, entradas);
    return; 
}
void inicializarVariablesGlobales(int sizeTabla, int qNiveles){
    tablaDeProcesos = list_create();
    maximoEntradasTabla = sizeTabla;
    nivelesTablas = qNiveles;
    // TODO: Inicializar void* memoriaDeUsuario segun el tamaño
}

/////////////////// TP FIN /////////////////////
