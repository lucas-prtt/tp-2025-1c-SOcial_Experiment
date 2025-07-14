#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <commons/collections/list.h>
void threadCancelAndDetach(pthread_t * hilo);
void closeTreadsFromListAndCleanUpList(void * list);
void cancelThreadByPointer(void * thread);
void joinTreadsFromListAndCleanUpList(void * list);
void joinThreadByPointer(void * thread);
void threadCancelAndJoin(pthread_t * hilo);

