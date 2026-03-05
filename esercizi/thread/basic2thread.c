#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define N 5
void* stampa(void* arg){
    int val = *(int*)arg;
    pthread_t current = pthread_self();
    fprintf(stdout,"Index: %d Thread: %lu.\n",val,(unsigned long)current);
    return NULL;
}


int main(int argc, char **argv){
    int *elements = malloc(sizeof(int)*N);
    pthread_t ths[N];


    for(int i = 0; i < N; i++){
        elements[i] = i;
        if((pthread_create(&ths[i],NULL,stampa,&elements[i]))!=0){
            perror("pthread_create:");
            exit(EXIT_FAILURE);
        }
    }


    for(int i = 0; i < N; i++){
        pthread_join(ths[i],NULL);
    }
    free(elements);
    exit(EXIT_SUCCESS);
}