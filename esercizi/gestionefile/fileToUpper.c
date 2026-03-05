#define MODE 0644
#define BUFFER_SIZE 2048


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>

void convert(char *input, char *output){
    int fi, fo, size;
    char buffer[BUFFER_SIZE];

    if((fi = open(input,O_RDONLY, MODE)) == -1){
        perror("FILE COULD NOT BE OPENED");
        exit(EXIT_FAILURE);
    }

    if((fo = open(output,O_CREAT | O_TRUNC | O_WRONLY,MODE)) == -1){
        perror("FILE COULD NOT BE OPENED");
        close(fi);
        exit(EXIT_FAILURE);
    }

    while((size = read(fi,buffer,BUFFER_SIZE)) > 0){
        for(int i = 0; i < size; i++){
            if(isalpha(buffer[i])){
                buffer[i] = toupper(buffer[i]);
            }
        }

        if(write(fo,buffer,size) != size){
            perror("ERROR WHILE WRITING");
            close(fi);
            close(fo);
            exit(EXIT_FAILURE);
        }
    }

    if(size == -1){
        perror("ERROR WHILE READING");
        close(fi);
        close(fo);
        exit(EXIT_FAILURE);
    }
    close(fi);
    close(fo);
    fprintf(stdout,"CONVERSION WITH SUCCESS.\n");
}

int main(int argc, char **argv){
    if(argc != 3){
        fprintf(stderr,"ERROR, INVALID NUMBER OF ARGUES");
        exit(EXIT_FAILURE);
    }
    convert(argv[1],argv[2]);
    return 0;
}