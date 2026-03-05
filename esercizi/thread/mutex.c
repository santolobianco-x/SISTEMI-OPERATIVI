#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


#define MAX 256
typedef struct {
    int id;
    char *key;
    FILE *fout;
    pthread_mutex_t *mtx;
} thread_arg;



void *thread_func(void *arg){
    thread_arg *t = arg;
    pthread_mutex_lock(t->mtx);
    fprintf(stdout,"[THREAD %d] writing...\n", t->id);
    fprintf(t->fout, "THREAD %d: %s\n", t->id, t->key);
    fflush(t->fout);
    pthread_mutex_unlock(t->mtx);
    return NULL;
}

int main(int argc, char **argv){
    char *keys[] = {"uno", "due", "tre", "quattro"};
    int n = 4;

    pthread_t tid[n];
    thread_arg args[n];

    FILE *fout = fopen("output.txt", "w");
    if(!fout){
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_t mtx;
    pthread_mutex_init(&mtx, NULL);

    for(int i = 0; i < n; i++){
        args[i].id = i;
        args[i].key = keys[i];
        args[i].fout = fout;
        args[i].mtx = &mtx;
        
        pthread_create(&tid[i], NULL, thread_func, &args[i]);
    }

    for(int i = 0; i< n; i++){
        pthread_join(tid[i], NULL);

    }
    pthread_mutex_destroy(&mtx);
    fclose(fout);
    return 0;
}