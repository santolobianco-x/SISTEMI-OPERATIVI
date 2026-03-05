#define MODE 0644
#define BUFFER_SIZE 2048
/***SI RICORDA CHE DEVE ESSERE GIA' PRESENTE UN FILE DI TESTO***/


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>


int counter(char *source){
    int fin;
    int size;
    int counter = 0;
    char buffer[BUFFER_SIZE];

    if((fin = open(source,O_RDONLY)) == -1){
        perror("FILE COULD NOT BE OPENED.");
        exit(EXIT_FAILURE);
    }
    
    while((size = read(fin,buffer,BUFFER_SIZE))>0){
        for(int i = 0; i < size; i++){
            if(buffer[i] == '\n'){
                counter++;
            }
        }
    }

    if(size == -1){
        perror("ERROR WHILE READING");
        close(fin);
        exit(EXIT_FAILURE);
    }

    return counter;
}

int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr,"INVALID NUMBER OF ARGUES.\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout,"THE NUMBER OF \\n IS: %d.\n",counter(argv[1]));
    exit(EXIT_SUCCESS);
}