#include "variablesGlobales.h"

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

void asignarMarcoAPagina(int numeroMarco, void ** arbolDePaginas, int maximoEntradasTabla, t_list * entradas, int niveles)
{
    int * ubicacion = buscarOCrear (arbolDePaginas, entradas, niveles, maximoEntradasTabla);
    *ubicacion = numeroMarco;
}
int leerMarcoDePagina(int numeroMarco, void ** arbolDePaginas, int maximoEntradasTabla, t_list * entradas, int niveles)
{
    int * ubicacion = buscarOCrear (arbolDePaginas, entradas, niveles, maximoEntradasTabla);
    return * ubicacion;
}
void liberarArbolDePaginas(void ** arbolDePaginas, int maximoEntradasTabla, int nivelesRestantes){
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
            liberarArbolDePaginas((void**)(arbolDePaginas[i]), maximoEntradasTabla, nivelesRestantes-1);
    }   
    free(arbolDePaginas);
    return;
}