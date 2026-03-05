#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


#define MODE 0644


void copy(int fin, int fout, ssize_t length){
    char *inbuf, *outbuf;
    if((inbuf =mmap(NULL,length,PROT_READ,MAP_PRIVATE,fin,0)) == MAP_FAILED){
        perror("mmap1");
        exit(EXIT_FAILURE);
    }

    if((outbuf = mmap(NULL,length,PROT_WRITE ,MAP_SHARED,fout,0)) == MAP_FAILED){
        perror("mmap2");
        exit(EXIT_FAILURE);
    }


    for(int i = 0; i < length; i++){
        outbuf[i] = inbuf[i];
    }


    msync(outbuf,length,MS_ASYNC);
    munmap(inbuf,length);
    munmap(outbuf,length);
}


int main(int argc, char ** argv){
    if(argc != 3){
        fprintf(stderr,"USAGE: %s <path> <path>\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    int fin = open(argv[1],O_RDONLY);
    int fout = open(argv[2], O_CREAT | O_TRUNC |  O_RDWR,MODE);
    //CON MMAP QUANDO SI CREA UN FILE, I PERMESSI ESPLICITATI DURANTE LA CREAZIONE
    //DEVONO ESSERE OBLIGATORIAMENTE DI READ E WRITE(NON SOLO UNO)

    if(fin == -1 || fout == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }

    struct stat sf;
    if(fstat(fin,&sf) == -1){
        perror("fstat");
        exit(EXIT_FAILURE);
    }

    ssize_t length = sf.st_size;

    if(ftruncate(fout,length) == -1){
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    copy(fin,fout,length);
    close(fin);
    close(fout);
    exit(EXIT_SUCCESS);
}