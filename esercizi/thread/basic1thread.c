#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


void* verifica(void* arg){
    pthread_t t1 = pthread_self();
    pthread_t t2 = *(pthread_t *)arg;

    if(pthread_equal(t1,t2) == 0){
        fprintf(stdout,"I thread sono diversi\n");
        fprintf(stdout,"T1: 0x%lx\n",(unsigned long)t1);
        fprintf(stdout,"T2: 0x%lx\n",(unsigned long)t2);
    }else{
        fprintf(stdout,"I thread sono uguali\n");
        fprintf(stdout,"T1: 0x%lx",(unsigned long)t1);
        fprintf(stdout,"T2: 0x%lx",(unsigned long)t2);
    }
    return NULL;
}


int main(){
    pthread_t t;
    pthread_t main_thread = pthread_self();
    fprintf(stdout,"Main Thread = 0x%lx\n",(unsigned long)main_thread);
    if((pthread_create(&t,NULL,verifica,&main_thread))!= 0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    sleep(1);
    return 0;
}