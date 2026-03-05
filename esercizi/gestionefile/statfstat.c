#define MODE 0644
#define BUFFER_SIZE 2048


#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>


int main(int argc, char **argv){


    struct stat buf;
    // si usa stat() quando si ha solo il percorso del file
    if(stat("esempio.txt",&buf) == -1){
        perror("ERROR WHILE ANALYZING FILE");
        exit(EXIT_FAILURE);
    }
    // altrimenti si può usare fstat se si ha il fd legato al file
    fprintf(stdout,"Dimension: %lld byte\n",buf.st_size);
    fprintf(stdout,"Permission: %o \n", buf.st_mode);
    fprintf(stdout,"Blocks: %lld \n", buf.st_blocks);
    fprintf(stdout,"Last access: %s \n\n", ctime(&buf.st_atime));


    struct stat buf1;
    int fd = open("esempio.txt", O_RDONLY);
    if(fd == -1){
        perror("FILE COULD NOT BE OPENED");
        exit(EXIT_FAILURE);
    }

    if(fstat(fd,&buf1) == -1){
        perror("ERROR WHILE ANALYZING FILE");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout,"Dimension: %lld byte\n",buf1.st_size);
    fprintf(stdout,"Permission: %o \n", buf1.st_mode);
    fprintf(stdout,"Blocks: %lld \n", buf1.st_blocks);
    fprintf(stdout,"Last access: %s \n\n", ctime(&buf1.st_atime));

    close(fd);
}
/*
SI UTILIZZA LSTAT PER VEDERE INFORMAZIONI DI FILE SIMBOLICI
(NON LI ATTRAVERSA COME FSTAT E STAT)
I FILE SIMBOLICI SONO DEI FILE CHE PUNTANO AD ALTRI FILE
*/