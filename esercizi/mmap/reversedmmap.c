#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>


typedef struct{
    char *data;
    size_t start;
    size_t length;
    size_t total_size;
    int id;
} thread_args;


void swap(char *a, char *b){
    char tmp = *a;
    *a = *b;
    *b = tmp;
}


void *reverse_chunk(void *arg){
    thread_args *t = (thread_args *) arg;
    for(size_t i = t->start; i < t->start + t->length; i++){
        swap(&t->data[i], &t->data[t->total_size-1-i]);
    }

    pthread_exit(NULL);
}


int main(int argc, char **argv){
    if(argc < 3){
        fprintf(stderr, "INVALID NUMBER OF ARGUMENTS. USAGE %s file num_threads\n", argv[1]);
        exit(EXIT_FAILURE);
    }


    int n_threads = atoi(argv[2]);
    if(n_threads <= 0){
        n_threads =1;
    }

    int fd = open(argv[1],O_RDWR);

    if(fd < 0){
        perror("open");
        exit(EXIT_FAILURE);
    }

    struct stat st;
    if(fstat(fd, &st) < 0){
        perror("fstat");
        close(fd);
        exit(EXIT_FAILURE);
    }

    size_t size = st.st_size;
    if(size < 2){
        fprintf(stderr,"FILE SIZE SMAL\n");
        close(fd);
        return 0;
    }


    char *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(data == MAP_FAILED){
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    size_t half = size/2;
    if(n_threads > half){
        n_threads = half;
    }
    size_t chunk = half / n_threads;
    size_t remainder = half % n_threads;

    pthread_t *threads = malloc(n_threads* sizeof(pthread_t));
    thread_args *targs = malloc(n_threads * sizeof(thread_args));

    size_t cursor = 0;

    for(int t = 0; t < n_threads; t++){
        size_t extra = (t < remainder) ? 1 : 0;
        targs[t].id = t;
        targs[t].data = data;
        targs[t].start = cursor;
        targs[t].length = chunk+extra;
        targs[t].total_size = size;
        pthread_create(&threads[t], NULL, reverse_chunk, &targs[t]);
        cursor += chunk+extra;
    }

    for(int t = 0; t < n_threads; t++){
        pthread_join(threads[t], NULL);
    }


    msync(data, size, MS_SYNC);
    munmap(data, size);


    free(threads);
    free(targs);
    fprintf(stdout,"FILE %s REVERSED BY %d THREADS.\n", argv[1], n_threads);
    return 0;
}