#include "threads.h"
void threadCancelAndDetach(pthread_t * hilo){
    pthread_cancel(*hilo);
    pthread_detach(*hilo);
}

void closeTreadsFromListAndCleanUpList(void * list){
    list_iterate(list, cancelThreadByPointer);
    list_destroy_and_destroy_elements(list, free);
}
void cancelThreadByPointer(void * thread){
    pthread_cancel(*(pthread_t*)thread);
}