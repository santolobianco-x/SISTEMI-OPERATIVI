#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define MODE 0644
#define BUFFERSIZE 20


/*

Istruzioni
Crea un file di testo data.txt con 20 righe numerate (Riga 1 … Riga 20).
Apri il file in lettura-scrittura usando open (Parte 1).
Leggi e stampa il contenuto del file con read + write su stdout.
Mappa tutto il file in memoria con mmap e MAP_SHARED (Parte 2).
Modifica:
prima metà delle righe → cambia ogni lettera a in A
seconda metà delle righe → sostituisci i con I
Sincronizza la mappa con msync.
Chiudi la mappa con munmap.
Riapri il file e stampalo di nuovo con read per verificare che le modifiche siano state salvate su disco.

*/


int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr,"USAGE: %s <filename>\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    

    int fd = open(argv[1],O_CREAT | O_TRUNC | O_WRONLY,MODE);

    if(fd == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFERSIZE];
    for(int i = 1; i <= 20; i++){
        sprintf(buffer,"Riga %d\n", i);
        if(write(fd,buffer,strlen(buffer)) != strlen(buffer)){
            perror("write");
        }
    }

    close(fd);

    if((fd = open(argv[1],O_RDWR,MODE)) == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }
    
    ssize_t breaded = 0;
    fprintf(stdout,"DIRECT READ FROM FILE:\n");
    while((breaded = read(fd,buffer,BUFFERSIZE)) > 0){
        if(write(STDOUT_FILENO,buffer,breaded) != breaded){
            perror("write");
        }
    }
    if(breaded == -1){
        perror("read");
        exit(EXIT_FAILURE);
    }


    struct stat sf;
    if(fstat(fd,&sf) == -1){
        perror("fstat");
        exit(EXIT_FAILURE);
    }

    ssize_t length = sf.st_size;
    ssize_t half = length/2;

    char *fileonram;
    if((fileonram = mmap(NULL,length,PROT_READ | PROT_WRITE, MAP_SHARED,fd,0)) == MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < half; i++){
        if(fileonram[i] == 'a'){
            fileonram[i] = 'A';
        }
    }

    for(int i = half; i < length; i++){
        if(fileonram[i] == 'i'){
            fileonram[i] = 'I';
        }
    }

    if(msync(fileonram,length,MS_SYNC) == -1){
        perror("msync");
        exit(EXIT_FAILURE);
    }

    munmap(fileonram,length);
    close(fd);

    if((fd = open(argv[1],O_RDONLY,MODE)) == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout,"READ FILE AFTER THE CHANGES:\n");
    while((breaded = read(fd,buffer,BUFFERSIZE))> 0){
        if(write(STDOUT_FILENO,buffer,breaded) != breaded){
            perror("write");
        }
    }

    if(breaded == -1){
        perror("read");
    }

    close(fd);
    exit(EXIT_SUCCESS);
    
}