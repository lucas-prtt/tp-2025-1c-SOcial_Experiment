#include <commons/collections/list.h>
#include "utils/logConfig.h"
#include <string.h>

typedef struct {
    int pagina;
    int marco;
    int validez;
    int timestamp;
} r_TLB;

typedef enum {
    TLB_HIT,
    TLB_MISS,
    CACHE_HIT,
    CACHE_MISS,
} HIT_OR_MISS;

enum TIPO_ALGORITMO_REEMPLAZO {
    ALG_FIFO,
    ALG_LRU,
    ALG_CLOCK,
    ALG_CLOCK_M,
    ERROR_NO_ALG,
};


enum TIPO_ALGORITMO_REEMPLAZO algoritmo_string_to_enum(char *nombreAlgoritmo);