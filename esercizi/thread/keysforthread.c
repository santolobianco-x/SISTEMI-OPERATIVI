#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>

#define MODE 0644
#define BUFFER_SIZE 2048
#define KEY_LENGTH 28
#define FILE_NAME_LENGTH 256
struct data{
    char** keys;
    int n;
};
typedef struct data data;


int rowcounter(int fd){
    int counter = 0; 
    if(lseek(fd,0,SEEK_SET) == -1){
        perror("lseek");
        exit(EXIT_FAILURE);
    }
    char buffer[BUFFER_SIZE];
    int size = 0; 
    while((size = read(fd,buffer,BUFFER_SIZE))> 0 ){
        for(int i = 0; i < size; i++){
            if(buffer[i] == '\n'){
                counter++;
            }
        }
    }
    if(size == -1){
        perror("read");
        exit(EXIT_FAILURE);
    }
    if(lseek(fd,0,SEEK_SET) == -1){
        perror("lseek");
        exit(EXIT_FAILURE);
    }
    return counter;
}

void* readkeys(void* arg){
    char* filename = (char *)arg;
    int fd;
    if((fd = open(filename,O_RDONLY,MODE)) == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }

    data* d = malloc(sizeof(data));
    if(d == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    d->n = rowcounter(fd);
    d->keys = malloc(sizeof(char*)*d->n);

    if(d->keys == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < d->n; i++){
        d->keys[i] = malloc(sizeof(char)*KEY_LENGTH);
        if(d->keys[i] == NULL){
            perror("malloc");
            exit(EXIT_FAILURE);
        }
    }
    FILE* f = fdopen(fd,"r");
    if(f == NULL){
        perror("fdopen");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < d->n; i++){
        if(fgets(d->keys[i],KEY_LENGTH,f)== NULL){
            perror("fgets");
            exit(EXIT_FAILURE);
        }
    }

    close(fd);

    pthread_exit((void *)d);
}

void* writekeys(void* arg){
    char* key = (char *) arg;
    pthread_t t = pthread_self();
    char filename[FILE_NAME_LENGTH];
    sprintf(filename,"%s%lu%s","output_",(unsigned long)t,".txt");
    int fd; 
    if((fd = open(filename,O_CREAT | O_TRUNC | O_WRONLY,MODE))== -1){
        perror("open");
        exit(EXIT_FAILURE);
    }

    if((write(fd,key,strlen(key))) != strlen(key)){
        perror("write");
        exit(EXIT_FAILURE);
    }
    printf("Thread %lu creato con chiave: %s\n", (unsigned long)t, key);
    close(fd);
    return NULL;
}



int main(int argc, char **argv){
    if(argc < 2){
        fprintf(stderr,"INVALID NUMBER OF ARGUMENTS. USAGE: %s <file>\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    pthread_t pt;

    if(pthread_create(&pt,NULL,readkeys,argv[1]) != 0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    void* returned;

    if(pthread_join(pt,&returned) != 0){
        perror("pthread_join");
        exit(EXIT_FAILURE);
    }

    data* da = (data*) returned;
    pthread_t ts[da->n];

    for(int i = 0; i < da->n; i++){
        if(pthread_create(&ts[i],NULL,writekeys,da->keys[i]) != 0){
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }


    for(int i = 0; i < da->n; i++){
        if((pthread_join(ts[i],NULL)) != 0){
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }

    free(da->keys);
    free(da);
}