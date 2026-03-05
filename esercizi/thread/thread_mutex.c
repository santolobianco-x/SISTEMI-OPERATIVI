#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define ITER 100000


typedef struct{
    int counter;
    pthread_mutex_t *mtx;
} thread_args ;


void *sync_increment(void *arg){
    thread_args *targs = (thread_args *) arg;
    for(int i = 0; i < ITER; i++){
        pthread_mutex_lock(targs->mtx);
        targs->counter++;
        pthread_mutex_unlock(targs->mtx);
    }
    return NULL;
}


void *increment(void *arg){
    thread_args *targs = (thread_args *) arg;
    for(int i = 0; i < ITER; i++){
        targs->counter++;
    }
    return NULL;
}

int main(int argc, char **argv){
    pthread_t t1, t2;
    pthread_mutex_t mtx;
    pthread_mutex_init(&mtx, NULL);
    thread_args targsync = {.counter = 0, .mtx = &mtx};
    thread_args targ = {.counter = 0, .mtx = NULL};
    pthread_create(&t1, NULL, sync_increment, &targsync);
    pthread_create(&t2, NULL, sync_increment, &targsync);

    pthread_join(t1,NULL);
    pthread_join(t2,NULL);

    pthread_mutex_destroy(&mtx);


    pthread_create(&t1, NULL, increment, &targ);
    pthread_create(&t2, NULL, increment, &targ);

    pthread_join(t1,NULL);
    pthread_join(t2,NULL);


    fprintf(stdout,"VALORE DI 'counter' CON SINCRONIZZAZIONE THREAD: %d\n", targsync.counter);
    fprintf(stdout,"VALORE DI 'counter' SENZA SINCRONIZZAZIONE THREAD: %d\n", targ.counter);
    return 0;

}
