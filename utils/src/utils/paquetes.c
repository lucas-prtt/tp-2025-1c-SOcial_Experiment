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

int recibir_paquete(int socket, t_paquete* paquete) {
    int tipo_mensaje;
    int tamanio;
    recv(socket, &tipo_mensaje, sizeof(int), MSG_WAITALL); // guarda en tipo_mensaje el encabezado del paquete leyendo los primeros 4 bytes del socket
    recv(socket, &tamanio, sizeof(int), MSG_WAITALL); // guarda los siguientes 4 bytes que representan al tamanio del buffer
    void* buffer = malloc(tamanio); // reserva memoria para el contenido
    recv(socket, buffer, tamanio, MSG_WAITALL); // recibe los bytes del socket y los guarda en el buffer
    paquete->tipo_mensaje = tipo_mensaje; // se cargan los valores recibidos en el paquete que se paso como parametro
    paquete->tamanio = tamanio;
    paquete->buffer = buffer;
}

void eliminar_paquete(t_paquete* paquete) { // libera la memoria utilizada por el paquete
    free(paquete->buffer);
    free(paquete);
}