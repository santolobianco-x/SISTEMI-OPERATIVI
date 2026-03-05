#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>


#define N 5



typedef struct{
    int *buffer;
    int in;
    int out;
    int count;
    pthread_mutex_t *mtx;
    pthread_cond_t *notfull;
    pthread_cond_t *notempty;
} thread_arg;


void *producer(void *arg){
    thread_arg *targ = (thread_arg *) arg;
    for(int i = 0; i < 10; i++){
        pthread_mutex_lock(targ->mtx);
        while(targ->count == N){
            pthread_cond_wait(targ->notfull, targ->mtx);
        }
        targ->buffer[targ->in] = i;
        targ->in = (targ->in +1) % N;
        targ->count++;

        fprintf(stdout,"[PRODUCER] INSERTED %d\n", i);
        pthread_cond_broadcast(targ->notempty);
        pthread_mutex_unlock(targ->mtx);
        usleep(rand()%500000);
    }
    pthread_exit(NULL);
}

void *consumer(void *arg){
    thread_arg *targ = (thread_arg *)arg;
    for(int i = 0; i < 2; i++){
        pthread_mutex_lock(targ->mtx);
        while(targ->count == 0){
            pthread_cond_wait(targ->notempty, targ->mtx);
        }
        int val = targ->buffer[targ->out];
        targ->count--;
        targ->out = (targ->out + 1)%N;
        fprintf(stdout,"[CONSUMER 0x%lu] FETCHED %d\n", (unsigned long) pthread_self(), val);
        pthread_cond_signal(targ->notfull);
        pthread_mutex_unlock(targ->mtx);
        usleep(rand()%5000);
    }
    pthread_exit(NULL);
}


int main(int argc, char **argv){
    int *buffer = malloc(sizeof(int)*N);
    pthread_mutex_t mtx;
    pthread_mutex_init(&mtx, NULL);
    pthread_cond_t notfull, notempty;
    pthread_cond_init(&notfull, NULL);
    pthread_cond_init(&notempty, NULL);


    thread_arg targ;
    targ.buffer = buffer;
    targ.in = 0; 
    targ.count = 0;
    targ.out = 0;
    targ.mtx = &mtx;
    targ.notfull = &notfull;
    targ.notempty = &notempty;

    pthread_t prod;
    pthread_t *cons = malloc(N*sizeof(pthread_t));

    pthread_create(&prod, NULL, producer, &targ);

    for(int i = 0; i < N; i++){
        pthread_create(&cons[i], NULL, consumer, &targ);
    }

    
    for(int i = 0; i < N; i++){
        pthread_join(cons[i], NULL);
    }
    pthread_join(prod, NULL);


    fprintf(stdout,"[MAIN] EXITING");
    return 0;
}