#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>


#define NUM_THREADS 5
#define INC_FOR_THREAD 100000

struct incr{
    unsigned long remanent_incr;
    unsigned long *shared;
    pthread_t t;
};

typedef struct incr incr;

unsigned long total = 0;



void* func(void *arg){
    incr* strinc = (incr *)arg;
    strinc->t = pthread_self();
    fprintf(stdout,"Il thread id: %lu sta incrementando...\n",(unsigned long)strinc->t);
    while(strinc->remanent_incr > 0){
        strinc->remanent_incr--;
        (*strinc->shared)++;
    }
    return ((void *)*strinc->shared);
}


int main(int argc, char **argv){
    pthread_t pts[NUM_THREADS];
    incr arrinc[NUM_THREADS];
    unsigned long expected = NUM_THREADS*INC_FOR_THREAD;
    for(int i = 0; i < NUM_THREADS; i++){
        arrinc[i].remanent_incr = INC_FOR_THREAD;
        arrinc[i].shared = &total;
        if(pthread_create(&pts[i],NULL,func,&arrinc[i]) != 0){
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }

    }


    for(int i = 0; i < NUM_THREADS;i++){
        void *returned;
        if(pthread_join(pts[i],&returned) != 0){
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
        fprintf(stdout,"Total: %lu after %lu.\n",(unsigned long)returned,(unsigned long)arrinc[i].t);
    }

    fprintf(stdout,"Total increment: %lu expected: %lu\n",total,expected);
}