#include <commons/collections/list.h>
#include "utils/logConfig.h"
#include <string.h>


//tengo que hacer el tamaño una variable global.

typedef struct {
    int pagina;
    int marco;
    int validez; // Para saber si el regitro de la tlb está vacia
    int ultimo_uso;
} r_TLB;

typedef struct {
    r_TLB entradas[3]; //hardcodeado
    int proximo; // FIFO
    int contador_uso; // LRU
    int algoritmo;
} TLB;

/*typedef enum {
    TLB_HIT,
    TLB_MISS,
} tlb_result;*/

enum TIPO_ALGORITMO_REEMPLAZO {
    ALG_FIFO,
    ALG_LRU,
    ALG_CLOCK,
    ALG_CLOCK_M,
    ERROR_NO_ALG,
};


enum TIPO_ALGORITMO_REEMPLAZO algoritmo_string_to_enum(char *nombreAlgoritmo);