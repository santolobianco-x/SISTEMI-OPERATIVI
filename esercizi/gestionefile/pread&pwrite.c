#define MODE 0644
#define BUFFER_SIZE 2048


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

void copyNBytes(char *source, char *dest){
    int fin, fout;
    if((fin = open(source,O_RDONLY,0644)) == -1){
        perror("INPUT FILE COULD NOT BE OPENED");
        exit(EXIT_FAILURE);
    }

    if((fout = open(dest,O_CREAT | O_TRUNC | O_WRONLY,0644)) == -1){
        perror("OUTPUT FILE COULD NOT BE OPENED");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    int rsize = 0;

    if((rsize = pread(fin,buffer,90,10)) < 0){
        perror("ERROR WHILE READING");
        close(fin);
        close(fout);
        exit(EXIT_FAILURE);
    }
    buffer[rsize] = '\0';

    if((pwrite(fout,buffer,rsize,0)) != rsize){
        perror("ERROR WHILE WRITING");
        close(fin);
        close(fout);
        exit(EXIT_FAILURE);
    }

    int c1,c2;

    if((c1 = lseek(fin,0,SEEK_CUR)) != 0){
        perror("ERROR CURSER MOVED");
        close(fin);
        close(fout);
        exit(EXIT_FAILURE);
    }

    if((c2 = lseek(fout,0,SEEK_CUR)) != 0){
        perror("ERROR CURSER MOVED");
        close(fin);
        close(fout);
        exit(EXIT_FAILURE);
    }
    fprintf(stdout,"THE BYTES HAS BEEN WRITTEN WITH SUCCESS\n");
}



int main(int argc, char **argv){
    if(argc != 3){
        fprintf(stderr,"INVALID NUMBER OF ARGUMENTS");
        exit(EXIT_FAILURE);
    }

    copyNBytes(argv[1],argv[2]);
}