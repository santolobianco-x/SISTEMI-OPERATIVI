#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>



#define SUBTHREADS 3
#define PAUSEMAX 8
#define MAX_RETURN_VALUE 10


void* func(void* arg){
    int id = *(int*)arg;
    pthread_t t = pthread_self();
    fprintf(stdout,"Il subthread T%d è stato creato tid: %lu.\n",id,(unsigned long)t);

    int pause = rand()%PAUSEMAX+1;
    fprintf(stdout,"Il subthread T%d è stato sospeso per %d secondi...\n",id,
    pause);
    sleep(pause);

    unsigned long toret = rand()%MAX_RETURN_VALUE+1;
    fprintf(stdout,"Il subthread T%d ritornerà %lu.\n",id,toret);

    return ((void*)toret);
}


int main(void){
    srand(time(NULL));
    pthread_t threads[SUBTHREADS];
    pthread_t mt = pthread_self();
    fprintf(stdout,"Il primo thread è stato creato tid:%lu.\n",(unsigned long)mt);

    int *id = malloc(sizeof(int)*SUBTHREADS);
    if(id == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < SUBTHREADS; i++){
        id[i] = i;
        if((pthread_create(&threads[i],NULL,func,&id[i]))!=0){
            perror("thread_create:");
            exit(EXIT_FAILURE);
        }
    }

    


    for(int i = 0; i < SUBTHREADS; i++){
        fprintf(stdout,"Attentendo la fine dell'esecuzione del thread T%d...\n",i);
        void *returned;
        if((pthread_join(threads[i],&returned))!= 0){
            perror("pthread_join:");
            exit(EXIT_FAILURE);
        }
        fprintf(stdout,"Valore restituito dal thread T%d = %lu.\n",i,(unsigned long)returned);
    }

    free(id);
    exit(EXIT_SUCCESS);
}