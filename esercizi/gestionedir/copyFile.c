#define MODE 0644
#define BUFFER_SIZE 2048

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>



void copy(char *source, char *dest){
    int fin, fout;
    int size;
    char buffer[BUFFER_SIZE];


    fin = open(source,O_RDONLY,MODE);
    if(fin == -1){
        perror("FILE COULD NOT BE OPENED.");
        exit(EXIT_FAILURE);
    }

    fout = open(dest,O_CREAT | O_WRONLY | O_TRUNC, MODE);
    if(fout == -1){
        perror("FILE COULD NOT BE OPENED.");
        close(fin);
        exit(EXIT_FAILURE);
    }

    while((size = read(fin,buffer,BUFFER_SIZE)) > 0){
        if((write(fout,buffer,size)) != size){
            perror("ERROR WHILE WRITING.");
            close(fin);
            close(fout);
            exit(EXIT_FAILURE);
        }
    }

    if(size == -1){
        perror("ERROR WHILE READING");
        close(fin);
        close(fout);
        exit(EXIT_FAILURE);
    }

    
    close(fin);
    close(fout);
    fprintf(stdout,"COPIED WITH SUCCESS.\n");
}


int main(int argc, char **argv){
    if(argc != 3){
        perror("INVALID NUMBER OF ARGUES");
        exit(EXIT_FAILURE);
    }

    copy(argv[1],argv[2]);
    exit(EXIT_SUCCESS);
}