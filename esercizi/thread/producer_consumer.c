#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef struct{
    int data;
    int ready;
    pthread_mutex_t *mtx;
} thread_args;


void *producer(void *arg){
    thread_args *targ = (thread_args *) arg;
    for(int i = 0; i < 5; i++){
        pthread_mutex_lock(targ->mtx);
        targ->data = i;
        targ->ready = 1;
        fprintf(stdout, "[PRODUCER] data %d ready\n", targ->data);
        pthread_mutex_unlock(targ->mtx);
        sleep(1);
    }
    return NULL;
}

void *consumer(void *arg){
    thread_args *targ = (thread_args *) arg;
    for(int i = 0; i < 5; i++){
        int consumed = 0;
        while(!consumed){
            pthread_mutex_lock(targ->mtx);
            if(targ->ready == 1){
                fprintf(stdout, "[CONSUMER] DATA %d CONSUMED\n", targ->data);
                targ->ready = 0;   
                consumed = 1;
            }
            pthread_mutex_unlock(targ->mtx);
            usleep(100);
        }
    }
    return NULL;
}


int main(int argc, char **argv){
    pthread_t prod, cons;
    pthread_mutex_t mtx;
    pthread_mutex_init(&mtx, NULL);
    
    thread_args t = {.data=0, .ready=0, .mtx = &mtx};


    pthread_create(&prod, NULL, producer, &t);
    pthread_create(&cons, NULL, consumer, &t);

    pthread_join(prod, NULL);
    pthread_join(cons, NULL);
}