#include "threads.h"
void threadCancelAndDetach(pthread_t * hilo){
    pthread_detach(*hilo);
    pthread_cancel(*hilo);
}

void closeTreadsFromListAndCleanUpList(void * list){
    list_iterate(list, cancelThreadByPointer);
    list_destroy_and_destroy_elements(list, free);
}
void cancelThreadByPointer(void * thread){
    pthread_cancel(*(pthread_t*)thread);
}
void joinTreadsFromListAndCleanUpList(void * list){
    list_iterate(list, joinThreadByPointer);
    list_destroy_and_destroy_elements(list, free);
}
void joinThreadByPointer(void * thread){
    pthread_cancel(*(pthread_t*)thread);
    pthread_join(*(pthread_t*)thread, NULL);
}
void threadCancelAndJoin(pthread_t * hilo){
    pthread_cancel(*hilo);
    pthread_join(*hilo, NULL);
}