#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


#define MODE 0644


/*
Esercizio 2 – Lettura e scrittura concorrente simulata
Obiettivo: simulare l’accesso concorrente a un file tramite mmap senza usare ancora thread.
Istruzioni:
Crea un file di testo con 10 righe numerate.
Mappa il file due volte (MAP_SHARED) con due puntatori diversi.
Modifica la prima metà delle righe tramite il primo puntatore e la seconda metà tramite il secondo.
Sincronizza entrambe le aree con msync.
Verifica che tutte le modifiche siano effettive.
Sfida extra: prova a stampare il contenuto della memoria dopo ogni modifica per vedere l’effetto immediato.
*/



void concurrentwrite(int fd, int length){
    ssize_t pagesize = sysconf(_SC_PAGE_SIZE);
    off_t half = length/2;
    off_t offset = (half/pagesize)*pagesize;

    

    char *firstmiddle, *secondmiddle;

    int nbyte = half;

    if((firstmiddle = mmap(NULL,nbyte,PROT_READ | PROT_WRITE, MAP_SHARED,fd,0)) == MAP_FAILED){
        perror("mmap1");
        exit(EXIT_FAILURE);
    }

    if(offset == 0){nbyte = length;}

    if((secondmiddle = mmap(NULL,nbyte,PROT_READ | PROT_WRITE,MAP_SHARED,fd,offset)) == MAP_FAILED){
        perror("mmap2");
        exit(EXIT_FAILURE);
    }


    if(offset == 0){secondmiddle = secondmiddle+half;}

    
    for(int i = 0; i < half; i++){
        if(firstmiddle[i] != '\n'){
            firstmiddle[i] = '*';
        }
    }

    for(int i = 0; i < length-half; i++){
        if(secondmiddle[i] != '\n'){
            secondmiddle[i] = '-';
        }
    }


    msync(firstmiddle,half,MS_SYNC);
    msync(secondmiddle,nbyte,MS_SYNC);
}



int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr,"USAGE: %s <path>\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd = open(argv[1],O_RDWR,MODE);
    if(fd == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }
    struct stat sf;
    if(fstat(fd,&sf) == -1){
        perror("fstat");
        exit(EXIT_FAILURE);
    }

    ssize_t length = sf.st_size;
    concurrentwrite(fd,length);

}