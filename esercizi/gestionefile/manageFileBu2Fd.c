#define MODE 0644
#define BUFFER_SIZE 2048


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>




int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr,"INVALID NUMBER OF ARGUMENTS");
        exit(EXIT_FAILURE);
    }

    int fd = open(argv[1],O_CREAT | O_TRUNC | O_WRONLY,MODE);
    if(fd == -1){
        perror("FILE COULD NOT BE OPENED");
        exit(EXIT_FAILURE);
    }

    int dupFd = dup(fd);

    if(dupFd < 0){
        perror("DUPLICATION OF FILE DESCRIPTOR FAILED");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if(write(fd,"FIRST FILE DESCRIPTOR\n",22) != 22){
        perror("ERROR WHILE WRITING");
        close(fd);
        exit(EXIT_FAILURE);
    }



    if(write(dupFd,"SECOND FILE DESCRIPTOR\n",23) != 23){
        perror("ERROR WHILE WRITING");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
    close(dupFd);
    fprintf(stdout,"FILE WRITTEN WITH SUCCESS\n");
    exit(EXIT_SUCCESS);
}