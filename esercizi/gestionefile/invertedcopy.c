#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


#define BUFFER_SIZE 2048
#define MODE 0644


void invertedcopy(char *source, char *dest){
    int fin, fout;
    int size;
    char buffer[1];

    if((fin = open(source,O_RDONLY,MODE)) == -1){
        perror("open file");
        exit(EXIT_FAILURE);
    }

    if((fout = open(dest,O_CREAT | O_TRUNC | O_WRONLY,MODE)) == -1){
        perror("open file");
        exit(EXIT_FAILURE);
    }

    off_t file_size = lseek(fin,0,SEEK_END);

    if(file_size == -1){
        perror("lseek");
        exit(EXIT_FAILURE);
    }

    for(off_t pos = file_size-1; pos >= 0; pos--){
        if(lseek(fin,pos,SEEK_SET) == -1){
            perror("lseek");
            exit(EXIT_FAILURE);
        }
        if((size = read(fin,buffer,1)) <= 0){
            break;
        }
        if((write(fout,buffer,1) != 1)){
            perror("write");
            exit(EXIT_FAILURE);
        }
    }

    if(size == -1){
        perror("reading");
        exit(EXIT_FAILURE);
    }

    close(fin);
    close(fout);
    fprintf(stdout,"COPY WITH SUCCESS\n");
}

int main(int argc, char **argv){
    if(argc != 3){
        fprintf(stderr,"USAGE: %s <file1> <file2>\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    invertedcopy(argv[1],argv[2]);
}
