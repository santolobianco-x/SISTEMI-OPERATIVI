#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>

int main(int argc, char **argv){
    if(argc != 4){
        fprintf(stderr,"INVALID NUMBER OF ARGUMENT, USAGE: %s <file> <offset> <num_byte>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int offset = atoi(argv[2]);
    int nbyte = atoi(argv[3]);
    

    int fd = open(argv[1], O_RDONLY);
    if(fd == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }

    struct stat st;
    if(fstat(fd,&st) == -1){
        perror("fstat");
        exit(EXIT_FAILURE);
    }

    int size = st.st_size;

    if(offset > size || offset + nbyte > size){
        fprintf(stderr, "INVALID OFFSET FOR THE FILE LENGTH");
        exit(EXIT_FAILURE);
    }

    //PRENDE DIMENSIONE DELLE PAGINE
    int pagesize = sysconf(_SC_PAGESIZE);

    //ALLINEA OFFSET CON MULTIPLI DELLA DIMENSIONE DELLE PAGINE
    off_t aligned_off = (offset / pagesize) * pagesize;
    //PRENDE LA DIFFERENZA TRA L'OFFSET E L'OFFSET ALLINEATO TRA LE PAGINE
    // ES. (4100-4096) = 4 ]-> PUNTO DI PARTENZA DELL'OFFSET
    off_t diff = offset-aligned_off;


    char *data;
    //PRENDIAMO GLI ELEMENTI A PARTIRE DALL'INIZIO DELLA PAGINA FINO ALL'INIZIO DELL'OFFSET
    //E INOLTRE SI PRENDE LO SPAZIAMENTO
    data = mmap(NULL, nbyte + diff, PROT_READ, MAP_PRIVATE, fd, aligned_off);
    if(data == MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < nbyte; i++){
        putc(data[diff+i],stdout);
    }

    if(munmap(data,nbyte+diff) == -1){
        perror("munmap");
        exit(EXIT_FAILURE);
    }

    if(close(fd) == -1){
        perror("close");
        exit(EXIT_FAILURE);
    }

    return 0;
}