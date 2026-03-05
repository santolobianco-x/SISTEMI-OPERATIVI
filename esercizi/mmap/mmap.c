#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
/*
MMAP È UNA PRIMITIVA DEL SISTEMA OPERATIVO, CHE PERMETTE
DI MAPPARE UN FILE ALL'INTERNO DELLO SPAZIO DI INDIRIZZAMENTO.
ATTRAVERSO QUESTA TECNICA, QUINDI, SI EVITA DI SCRIVERE O LEGGERE COSTANTEMENTE DALLA MEMORIA,
CARICANDO IL CONTENUTO IN RAM.
SI LEGGE, SI MODIFICA E INFINE SI SALVA IN MEMORIA SE È PERMESSO(MAP SHARED).
POSSIAMO QUINDI AGGIRARE I COSTI DELLA LETTURA/SCRITTURA IN MEMORIA.
*/
int main(){
    int fd = open("mmapfile.txt", O_RDONLY);
    if(fd == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }

    struct stat fs;
    if(fstat(fd,&fs) == -1){
        perror("fstat");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout,"FILE DIMENSION: %lld bytes\n",fs.st_size);
    //SALVATAGGIO DEL CONTENUTO DEL FILE ALL'INTERNO DELL'ARRAY
    char *data = mmap(NULL,fs.st_size,PROT_READ,MAP_PRIVATE,fd,0);
    if(data == NULL){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    if(write(STDOUT_FILENO,data,fs.st_size) != fs.st_size){
        perror("write");
    }
    //LIBERAZIONE DELLA MEMORIA
    munmap(data,fs.st_size);
    close(fd);
    exit(EXIT_SUCCESS);
}
