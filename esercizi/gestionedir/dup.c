#define MODE 0644

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr,"Usage: %s <filename>",argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd = open(argv[1],O_CREAT | O_WRONLY | O_TRUNC, MODE);
    if(fd == -1){
        perror("FILE COULD NOT BE OPENED");
        exit(EXIT_FAILURE);
    }
    //con dup duplichiamo i file descriptor
    //in questo modo stiamo duplicando il file descriptor dello standard out
    int stdout_backup = dup(STDOUT_FILENO);
    //con dup2 duplichiamo il file descriptor di fd
    dup2(fd,STDOUT_FILENO);
    //possiamo chiudere fd e comunque scrivere al suo interno attraverso lo standard out
    close(fd);
    printf("ciao");
    fflush(stdout);

    dup2(stdout_backup,STDOUT_FILENO);
    close(stdout_backup);
    printf("sono santo");
    exit(EXIT_SUCCESS);
}