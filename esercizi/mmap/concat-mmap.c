#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/mman.h>
#include <pthread.h>
#include <string.h>


#define MODE 0644


typedef struct{
    char *first_map;
    char *second_map;
    int fout;
    struct stat st1;
    struct stat st2;
    int state;
} thread_args;

void *thread_func(void *args){
    thread_args *t = (thread_args *)args;
    int size = t->st1.st_size + t->st2.st_size;
    if(ftruncate(t->fout, size) == -1){
        fprintf(stderr, "[THREAD] FTRUNCATE ERROR, EXITING...\n");
        t->state = 1;
        return NULL;
    }

    struct stat st;
    if(fstat(t->fout, &st) == -1){
        fprintf(stderr,"[THREAD] FSTAT ERROR, EXITING...\n");
        t->state = 1;
        return NULL;
    }
    
    char *data;
    data = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, t->fout,0);
    if(data == MAP_FAILED){
        fprintf(stderr,"[THREAD] MMAP ERROR, EXITING...\n");
        t->state = 1;
        return NULL;
    }

    fprintf(stdout, "[THREAD] COPYING FIRST FILE\n");
    memcpy(data, t->first_map, t->st1.st_size);

    fprintf(stdout, "[THREAD] COPYING SECOND FILE\n");
    memcpy(data+t->st1.st_size, t->second_map, t->st2.st_size);

    fprintf(stdout, "[THREAD] SAVING... \n");
    if(msync(data, st.st_size, MS_SYNC) == -1){
        fprintf(stderr,"[THREAD] ERROR WHILE SAVING FILE\n");
        t->state = 1;
        return NULL;
    }
    fprintf(stdout, "[THREAD] FILE SAVED... \n");
    munmap(data, st.st_size);
    return NULL;
}

int main(int argc, char **argv){
    if(argc < 4){
        fprintf(stderr,"INVALID NUMBER OF ARGUMENTS. USAGE %s <inputfile> <inputfile> <outputfile>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int fi1, fi2, fout;
    fi1 = open(argv[1], O_RDONLY);
    fi2 = open(argv[2], O_RDONLY);
    fout = open(argv[3], O_CREAT | O_TRUNC | O_RDWR, MODE);
    if(fi1 == -1|| fi2 == -1|| fout == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }

    struct stat st1, st2;
    if(fstat(fi1, &st1) == -1 ||fstat(fi2, &st2) == -1){
        perror("fstat");
        exit(EXIT_FAILURE);
    }

    char *data1, *data2;

    data1 = mmap(NULL, st1.st_size, PROT_READ, MAP_PRIVATE, fi1, 0);
    data2 = mmap(NULL, st2.st_size, PROT_READ, MAP_PRIVATE, fi2, 0);
    if(data1 == MAP_FAILED || data2 == MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    thread_args targs = {.first_map=data1, .second_map=data2, .st1 = st1, .st2 = st2, .fout = fout, .state = 0};
    pthread_t t;

    if(pthread_create(&t, NULL, thread_func, &targs) != 0){
        perror("pthread_create");
        munmap(data1,st1.st_size);
        munmap(data2,st2.st_size);
        exit(EXIT_FAILURE);
    }

    pthread_join(t, NULL);
    if(targs.state == 1){
        fprintf(stderr, "[MAIN] ERROR FROM THREAD.\n");
        munmap(data1,st1.st_size);
        munmap(data2,st2.st_size);
        close(fi1);
        close(fi2);
        close(fout);
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "[MAIN] FILES COPIED WITH SUCCESS\n");
    munmap(data1,st1.st_size);
    munmap(data2,st2.st_size);
    close(fi1);
    close(fi2);
    close(fout);
    return 0;

}