#include "paquetes.h" // inclusion de las funciones y estructura del paquete
#include <string.h>
#include <sys/socket.h>

t_paquete* crear_paquete(int tipo_mensaje) {
    t_paquete* paquete = malloc(sizeof(t_paquete)); // reserva memoria para el nuevo paquete
    paquete->tipo_mensaje = tipo_mensaje; // agrega a la estructura del paquete el tipo de mensaje pasado como parametro
    paquete->buffer = NULL; // como al paquete todavia no se le agregó contenido, lo ponemos en nulo
    paquete->tamanio = 0; // como al paquete todavia no se le agregó contenido, lo ponemos en 0
    return paquete; 
}

void agregar_a_paquete(t_paquete* paquete, void* contenido, int size) {
    paquete->buffer = realloc(paquete->buffer, paquete->tamanio + size+sizeof(int)); // hace que el buffer tenga espacio para los datos a agregar + un entero que indica cuantos datos hay para leer
    memcpy((char*)paquete->buffer + paquete->tamanio, &size, sizeof(int)); //Se agrega un int que indica el tamaño del dato
    memcpy((char*)paquete->buffer + paquete->tamanio + sizeof(int), contenido, size); // Copia al buffer el mensaje a enviar
    paquete->tamanio = paquete->tamanio + size + sizeof(int); // aumenta el tamanio total del buffer al agregarle el tamaño de los datos y el tamaño de un int
}

void enviar_paquete(t_paquete* paquete, int socket) {
    int total_bytes = sizeof(int) + sizeof(int) + paquete->tamanio; //calcula el tamaño total del bloque
    void* buffer = malloc(total_bytes);// reserva memoria para el bloque
    int offset = 0; // variable desplazamiento para rellenar el buffer
    memcpy(buffer + offset, &(paquete->tipo_mensaje), sizeof(int)); // copia el tipo de mensaje en el principio del buffer
    offset += sizeof(int); //aumenta el uffset para dejar espacio al contenido
    memcpy(buffer + offset, &(paquete->tamanio), sizeof(int)); // copa el tamanio del buffer despues del tipo de mensaje
    offset += sizeof(int);
    memcpy(buffer + offset, paquete->buffer, paquete->tamanio); // copia el contenido del buffer despues de los dos int
    send(socket, buffer, total_bytes, 0); // envia el paquete
    free(buffer); // libera la memoria despues de enviarlo
}

void enviar_paquete_error(int socket_cliente, t_list *lista_contenido) {
    int error = -1;
    t_paquete *paquete_error = crear_paquete(HANDSHAKE);
    agregar_a_paquete(paquete_error, &error, sizeof(int));
    enviar_paquete(paquete_error, socket_cliente);
    eliminar_paquete(paquete_error);
    eliminar_paquete_lista(lista_contenido);
}

int recibir_paquete_bloqueante(int socket, t_paquete* paquete){
    return recibir_paquete(socket, paquete, MSG_WAITALL);
}

int recibir_paquete(int socket, t_paquete* paquete, int flags) {
    //retorna 0 al haber error o cierre de la conexion, 1 si todo va bien
    int tipo_mensaje;
    int tamanio;
    if(recv(socket, &tipo_mensaje, sizeof(int), flags) <= 0) return 0;
    if(recv(socket, &tamanio, sizeof(int), flags) <= 0) return 0;
    void* buffer = NULL;
    if(tamanio>0){
        buffer = malloc(tamanio);
        if(recv(socket, buffer, tamanio, flags) <= 0) 
        {
            free(buffer);
            return 0;
        }
    }
    paquete->tipo_mensaje = tipo_mensaje;
    paquete->tamanio = tamanio;
    paquete->buffer = buffer;
    return 1;
}

t_list *recibir_paquete_lista(int socket, int flags, int* codOp) { //CodOp se setea con el tipo de mensaje
    t_paquete * paq;
    paq = malloc(sizeof(t_paquete));
    if(!recibir_paquete(socket, paq, flags)) {
        free(paq);
        if(codOp != NULL)
        *codOp = -42;
        return NULL;
    }
    if(codOp != NULL) { // Si no me interesa el codOp del paquete, le mando null
        *codOp = paq->tipo_mensaje;
    }
    if(paq->tamanio == 0){
        eliminar_paquete(paq);
        return list_create();
    }
    t_list * listaDeContenido = list_create();
    int offset = 0;
    int * tamanioElemento;
    while(offset < paq->tamanio){
        tamanioElemento = malloc(sizeof(int));
        memcpy(tamanioElemento, paq->buffer + offset, sizeof(int)); // Copia el tamaño de el dato serializado que va a leer
        offset += sizeof(int);
        void * elemento = malloc(*tamanioElemento);
        memcpy(elemento, paq->buffer + offset, *tamanioElemento);
        offset += *tamanioElemento;
        list_add(listaDeContenido, tamanioElemento);
        list_add(listaDeContenido, elemento);
        //Crea una lista en la cual:
        /*
        Lista[0] = tamaño elemento 0
        lista[1] = pointer a elemento 0
        Lista[2] = tamaño elemento 1
        Lista[3] = pointer a elemento 1
        Lista[n*2] = tamaño elemento n
        Lista[n*2+1] = pointer a elemento n
        Si un tipo de dato es estatico (int/bool) , no hay problema, el tamaño nomas va a ser redundante
        Si un tipo de dato es dinamico (int[n]/char[n]), tenemos en la lista cual es la longitud del elemento
        */
    }
    eliminar_paquete(paq);
    return listaDeContenido;
}

void eliminar_paquete(t_paquete* paquete) { // libera la memoria utilizada por el paquete
    free(paquete->buffer);
    free(paquete);
}

void eliminar_paquete_lista(t_list * listaDelPaquete){
    if(listaDelPaquete != NULL){
        if(list_size(listaDelPaquete)>0)
        list_destroy_and_destroy_elements(listaDelPaquete, free);   
        else
        list_destroy(listaDelPaquete);
    }
}