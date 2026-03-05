#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#define MODE 0644

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


    ssize_t nbyte = sf.st_size;


    char *data;

    if((data = mmap(0,nbyte,PROT_READ | PROT_WRITE, MAP_SHARED,fd,0)) == MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    if(nbyte >= 5){
        for(int i = 0; i < 5; i++){
            data[i] = toupper(data[i]);
        }
    }
    //MS_SYNC E MS_ASYNC SONO DEI FLAG CHE PERMETTONO LA SCRITTURA
    //SINCRONA O ASINCRONA
    //SINCRONA -> LA FUNZIONE RITORNA DOPO AVER SCRITTO TUTTE LE INFORMAZIONI SUL DISCO
    //ASINCRONA -> LA FUNZIONE RITORNA SUBITO E ALLA FINE VERRANNO SCRITTE TUTTE LE INFORMAZIONI
    if(msync(data,nbyte,MS_ASYNC) == -1){
        perror("msync");
    }
    if(munmap(data,nbyte) == -1){
        perror("munmap");
    }

    close(fd);
    exit(EXIT_SUCCESS);
}