#include "threads.h"
int threadCancelAndDetach(pthread_t * hilo){
    pthread_cancel(*hilo);
    pthread_detach(*hilo);
    return 0;
}