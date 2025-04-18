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
int recibir_paquete_bloqueante(int socket, t_paquete* paquete){
    return recibir_paquete(socket, paquete, MSG_WAITALL);
}
int recibir_paquete(int socket, t_paquete* paquete, int flags) { //Previamente, siempre estaba en MSG_WAITALL, añadi parametro flags para que tambien pueda ser MSG_DONTWAIT u otros
    int tipo_mensaje;
    int tamanio;
    recv(socket, &tipo_mensaje, sizeof(int), flags); // guarda en tipo_mensaje el encabezado del paquete leyendo los primeros 4 bytes del socket
    recv(socket, &tamanio, sizeof(int), flags); // guarda los siguientes 4 bytes que representan al tamanio del buffer
    void* buffer = malloc(tamanio); // reserva memoria para el contenido
    recv(socket, buffer, tamanio, flags); // recibe los bytes del socket y los guarda en el buffer
    paquete->tipo_mensaje = tipo_mensaje; // se cargan los valores recibidos en el paquete que se paso como parametro
    paquete->tamanio = tamanio;
    paquete->buffer = buffer;
    return 0;
}

t_list * recibir_paquete_lista(int socket, int flags, int * codOp){ //CodOp se setea con el tipo de mensaje
    t_paquete * paq;
    paq = malloc(sizeof(paq));
    recibir_paquete(socket, paq, flags);
    t_list * listaDeContenido = list_create();
    int offset = 0;
    int * tamanioElemento;
    if(codOp != NULL){ // Si no me interesa el codOp del paquete, le mando null
        *codOp = paq->tipo_mensaje;
    }
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
    list_destroy_and_destroy_elements(listaDelPaquete, free);
}