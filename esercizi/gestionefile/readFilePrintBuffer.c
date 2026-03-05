#define BUFFER_SIZE 2048
#define MODE 0644
/***SI RICORDA CHE DEVE ESSERE GIA' PRESENTE UN FILE DI TESTO***/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>



int main(){
    int fd = open("output.txt",O_RDONLY,MODE);
    if(fd == -1){
        perror("ERRORE DURANTE L'APERTURA DEL FILE\n");
        exit(EXIT_FAILURE);
    }
    char buffer[BUFFER_SIZE];

    ssize_t bytes_read;
    while((bytes_read = read(fd,buffer,BUFFER_SIZE)) > 0){
        if(write(STDOUT_FILENO,buffer,bytes_read) == -1){
            perror("ERRORE DURANTE LA SCRITTURA DEL FILE\n");
            close(fd);
            exit(EXIT_FAILURE);
        }
    }

    if(bytes_read == -1){
        perror("ERRORE DURANTE LA LETTURA DEL FILE!\n");
        close(fd);
        exit(EXIT_FAILURE);
    }


    close(fd);
    exit(EXIT_SUCCESS);
}