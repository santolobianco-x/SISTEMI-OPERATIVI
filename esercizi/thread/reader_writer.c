#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "lib-misc.h"
#include <pthread.h>

#define N 10


typedef struct{
    int *arr;
    int id;
    int length;
    pthread_rwlock_t *rw;
} thread_arg;



void *reader(void *arg){
    thread_arg *targ = (thread_arg *)arg;
    usleep(rand()%500000);
    pthread_rwlock_rdlock(targ->rw);
    int index = rand()%targ->length;
    fprintf(stdout,"[READER %d] READED %d\n", targ->id, targ->arr[index]);
    pthread_rwlock_unlock(targ->rw);
    pthread_exit(NULL);
}



void *writer(void *arg){
    thread_arg *targ = (thread_arg *) arg;
    usleep(rand()%500000);
    pthread_rwlock_wrlock(targ->rw);
    for(int i = 0; i < targ->length; i++){
        targ->arr[i] = rand()%100+1;
        fprintf(stdout,"[WRITER %d] WRITED %d\n",targ->id ,targ->arr[i]);
    }
    pthread_rwlock_unlock(targ->rw);
    pthread_exit(NULL);
}


int main(int argc, char **argv){
    int arr[N];
    for(int i = 0; i < N; i++){
        arr[i] = rand()%100+1;
        fprintf(stdout,"%d", arr[i]);
    }

    pthread_t rds[100], wts[100];
    thread_arg rargs[100],wargs[100];

    pthread_rwlock_t rwt;
    pthread_rwlock_init(&rwt, NULL);

    for(int i = 0; i < 100; i++){
        wargs[i].arr = arr;
        wargs[i].length = N;
        wargs[i].rw = &rwt;
        wargs[i].id = i;
        pthread_create(&wts[i], NULL, writer, &wargs[i]);
    }
    
    for(int i = 0; i < 100; i++){
        rargs[i].id = i;
        rargs[i].arr = arr;
        rargs[i].length = N;
        rargs[i].rw = &rwt;
        pthread_create(&rds[i], NULL, reader, &rargs[i]);
    }

    for(int i= 0; i < 100; i++)
    pthread_join(wts[i], NULL);
    for(int i= 0; i < 100; i++)
    pthread_join(rds[i], NULL);

    return 0;
}