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
    paquete->buffer = realloc(paquete->buffer, paquete->tamanio + size); // hace que el buffer tenga espacio para los datos a agregar
    memcpy((char*)paquete->buffer + paquete->tamanio, contenido, size); // copia al buffer el mensaje a enviar
    paquete->tamanio += size; // aumenta el tamanio total del buffer al agregarle los datos
}

void enviar_paquete(t_paquete* paquete, int socket) {
    send(socket, &(paquete->tipo_mensaje), sizeof(int), 0); // envia el tipo de mensaje
    send(socket, &(paquete->tamanio), sizeof(int), 0); // envia el tamaño del buffer
    send(socket, paquete->buffer, paquete->tamanio, 0); // envia el contenido del buffer
}

void eliminar_paquete(t_paquete* paquete) { // libera la memoria utilizada por el paquete
    free(paquete->buffer);
    free(paquete);
}