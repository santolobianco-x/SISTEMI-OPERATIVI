#define MODE 0644
#define BUFFER_SIZE 2048


#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>



int main(int argc, char **argv){


    if(argc != 2){
        fprintf(stderr,"USE: %s <filename>.\n",argv[0]);
        exit(EXIT_FAILURE);
    }


    int fd = open(argv[1],O_CREAT | O_TRUNC | O_WRONLY,MODE);

    if(fd == -1){
        perror("FILE COULD NOT BE OPENED");
        close(fd);
        exit(EXIT_FAILURE);
    }

    //SCRITTURA SENZA MUOVERE IL CURSORE DEL FILE
    // SI SPECIFICANO QUANTI BYTE SI DEVONO SCRIVERE E DA DOVE INIZIARE A SCRIVERE
    //UTILI PER SISTEMI MULTI-THREADING
    pwrite(fd,"CIAO MI CHIAMO SANTO",15,100);
    int c = lseek(fd,0,SEEK_CUR);
    printf("THE CURSOR IS IN: %d",c);

}
