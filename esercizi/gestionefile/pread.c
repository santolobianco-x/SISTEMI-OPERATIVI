#define MODE 0644
#define BUFFER_SIZE 2048

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr,"Use: %s <filename>\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd = open(argv[1],O_RDONLY,MODE);
    int r1 = 0, r2 = 0;
    if(fd == -1){
        perror("FILE COULD NOT BE OPENED");
        exit(EXIT_FAILURE);
    }


    char buffer[BUFFER_SIZE];

    //LEGGE 5 BYTE SENZA MUOVERE IL CURSORE
    //UTILE IN CONTESTI MULTITHREADING
    //SI SPECIFICA LA POSIZIONE DI PARTENZA PER LA LETTURA E L'OFFSET
    if((r1 = pread(fd,buffer,5,0)) == -1){
        perror("ERROR WHILE READING");
        close(fd);
        exit(EXIT_FAILURE);
    }
    buffer[r1] = '\0';
    printf("FIRST READ: %s\n",buffer);


    if((r2 = pread(fd,buffer,5,5)) == -1){
        perror("ERROR WHILE READING");
        close(fd);
        exit(EXIT_FAILURE);
    }
    buffer[r2] = '\0';
    printf("SECOND READ: %s\n",buffer);

    //CON LSEEK VERIFICHIAMO LA POSIZIONE
    //SE LA LETTURA AVVIENE SENZA MUOVERE IL CURSORE, ALLORA AVREMO UN VALORE PARI A 0
    int position = lseek(fd,0,SEEK_CUR);
    printf("FILE POSITION: %d\n",position);
    exit(EXIT_SUCCESS);
}