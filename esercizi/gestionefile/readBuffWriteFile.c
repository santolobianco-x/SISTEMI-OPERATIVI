#define MODE 0644
#define BUFFER_SIZE 2048


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(){
    char text[BUFFER_SIZE];
    int fd = open("output.txt", O_WRONLY | O_APPEND | O_CREAT,MODE); //OPEN RITORNA IL FILE DESCRIPTOR
    //             NOME FILE        MODALITÀ                  PERMESSO                 
    if(fd == -1){
        perror("ERRORE DURANTE L'APERTURA DEL FILE\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout,"INSERISCI DUE RIGHE:\n");
    for(int i = 0; i < 2; i++){
        fgets(text,BUFFER_SIZE,stdin);
        ssize_t bytes_written = write(fd,text,strlen(text));
        if(bytes_written == -1){
            perror("ERRORE DURANTE LA SCRITTURA DEL FILE!\n");
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
    close(fd);
    exit(EXIT_SUCCESS);
}