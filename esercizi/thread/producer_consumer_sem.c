#include "lib-misc.h"
// DATA LA LIBRERIA 'lib-misc.h'(per semafori) NEL PROMPT DELLA COMPILAZIONE METTERE: ↓
// gcc -o *object-file* producer_consumer_sem.c lib-misc.c
// PER L'ESECUZIONE BASTA SOLO INDICARE: ./exec
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>


#define N 5


typedef struct{
    sem_t *full;
    sem_t *empty;
    int *buffer;
    int in;
    int out;
    pthread_mutex_t *mtx;
} thread_arg;

void *produce(void *arg){
    thread_arg *targ = (thread_arg *) arg;
    for(int i = 0; i < 10; i++){

        int source = rand()%100+1;

        sem_wait(targ->empty);
        pthread_mutex_lock(targ->mtx);


        targ->buffer[targ->in] = source;
        targ->in = (targ->in+1)%N;


        fprintf(stdout, "[PRODUCER] INSERT %d\n", source);

        pthread_mutex_unlock(targ->mtx);
        sem_post(targ->full);
        sleep(1);
    }
    pthread_exit(NULL);
}

void *consume(void *arg){
    thread_arg *targ = (thread_arg *) arg;
    for(int i = 0; i < 10; i++){
        int val; 
        sem_wait(targ->full);
        pthread_mutex_lock(targ->mtx);
        val = targ->buffer[targ->out];
        fprintf(stdout, "[CONSUMER] FETCHED %d\n", val);
        targ->out = (targ->out+1)%N;
        pthread_mutex_unlock(targ->mtx);
        sem_post(targ->empty);
        sleep(1);
    }
    pthread_exit(NULL);
}


int main(int argc, char **argv){

    pthread_t producer, consumer;
    pthread_mutex_t mtx;
    pthread_mutex_init(&mtx, NULL);
    sem_t full, empty;
    sem_init(&full, 0, 0);
    sem_init(&empty, 0, N);
    thread_arg arg = {
        .buffer =malloc(sizeof(int)*N), 
        .empty = &empty, 
        .full = &full, 
        .in = 0,
        .out = 0,
        .mtx = &mtx};

    pthread_create(&producer, NULL, produce, &arg);
    pthread_create(&consumer, NULL, consume, &arg);

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);

    pthread_mutex_destroy(&mtx);
    
    sem_destroy(&full);
    sem_destroy(&empty);
    free(arg.buffer);
    return 0;

    
}
