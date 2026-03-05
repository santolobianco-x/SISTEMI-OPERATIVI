#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>



#define BUFFER_SIZE 15
#define MODE 0644
#define LINE_LENGTH 4096


void printrow(int fd){
    int size = 0;
    char buffer[BUFFER_SIZE];
    char line[LINE_LENGTH];
    int currentlength = 0;
    int row = 1;
    char rowline[20];
    int rlength = sprintf(rowline,"RIGA %d ",row);


    if(write(STDOUT_FILENO,rowline,rlength) != rlength){
        perror("write");
        exit(EXIT_FAILURE);
    }


    while((size = read(fd,buffer,BUFFER_SIZE-1)) >0 ){
        for(int i = 0; i < size; i++){
            line[currentlength] = buffer[i];
            currentlength++;
            if(buffer[i] == '\n'){
                row++;
                rlength = sprintf(rowline,"RIGA %d ",row);
                if(write(STDOUT_FILENO,line,currentlength)!= currentlength){
                    perror("write");
                    exit(EXIT_FAILURE);
                }

                if(write(STDOUT_FILENO,rowline,rlength) != rlength){
                    perror("write");
                    exit(EXIT_FAILURE);
                }
                currentlength = 0;
            }
        }
    }
    if(currentlength > 0){
        if(write(STDOUT_FILENO, line, currentlength) != currentlength){
            perror("write");
            exit(EXIT_FAILURE);
        }
    }
    if(size == -1){
        perror("read");
        exit(EXIT_FAILURE);
    }
}


int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr,"INVALID NUMBER OF ARGUMENTS. USAGE: %s <file>\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd = open(argv[1],O_RDONLY,MODE);


    if(fd == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }

    printrow(fd);


    if(close(fd) == -1){ perror("close"); exit(EXIT_FAILURE);}
    exit(EXIT_SUCCESS);
}