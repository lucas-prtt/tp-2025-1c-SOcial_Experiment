//include guard para que no se incluya mas de una vez
#ifndef PAQUETE_H
#define PAQUETE_H 

#include <stdint.h>
#include <stdlib.h>
#include <commons/collections/list.h> //Para poder devolver paquete como lista
#include <utils/enums.h>

typedef struct { //defino la estructura que va a tener el paquete
    int tipo_mensaje; //indica si es un error, handshake, datos, etc.
    void* buffer; // puntero generico a los datos
    int tamanio; // el tamaño que van a tener esos datos
} t_paquete;

t_paquete* crear_paquete(int tipo_mensaje); // va a crear el paquete pasandole por parametro el tipo de mensaje a enviar
void agregar_a_paquete(t_paquete* paquete, void* contenido, int size); // se pasa por parametro el paquete, el contenido a enviar y el tamaño de datos del contenido
void enviar_paquete(t_paquete* paquete, int socket); // se pasa el paquete y el socket para poder enviarlo
void enviar_paquete_error(int socket_cliente, t_list *lista_contenido); //paquete de error, en caso de problemas en el handshake
void eliminar_paquete(t_paquete* paquete); // se elimina el paquete para librer la memoria
int recibir_paquete(int socket, t_paquete* paquete, int flags);
int recibir_paquete_bloqueante(int socket, t_paquete* paquete);
t_list * recibir_paquete_lista(int socket, int flags, int * codOp);
void eliminar_paquete_lista(t_list * listaDelPaquete);
#endif // finaliza el guard de inclusion