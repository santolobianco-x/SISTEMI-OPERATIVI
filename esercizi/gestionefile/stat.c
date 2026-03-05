#define MODE 0644
#define BUFFER_SIZE 2048

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>



void printFileType(char *filename ,mode_t type){
    if(S_ISREG(type)){
        fprintf(stdout,"%s IS A REGULAR FILE.\n",filename);
    } else if(S_ISDIR(type)){
        fprintf(stdout,"%s IS A DIRECTORY.\n",filename);
    } else if(S_ISFIFO(type)){
        fprintf(stdout,"%s IS A FIFO FILE.\n",filename);
    } else if(S_ISCHR(type)){
        fprintf(stdout,"%s IS A CHARACTER DEVICE.\n",filename);
    } else if(S_ISBLK(type)){
        fprintf(stdout,"%s IS A BLOCK DEVICE.\n",filename);
    }
    else if(S_ISLNK(type)){
        fprintf(stdout,"%s IS A SYMBOLIC LINK.\n",filename);
    }
     else{
        fprintf(stdout,"UNKNOWN TYPE.\n");
    }
}


void printPermission(mode_t mode){
    char perms[10];
    perms[0] = (mode & S_IRUSR)? 'r' : '-';
    perms[1] = (mode & S_IWUSR)? 'w' : '-';
    perms[2] = (mode & S_IXUSR)? 'x' : '-';
    perms[3] = (mode & S_IRGRP)? 'r' : '-';
    perms[4] = (mode & S_IWGRP)? 'w' : '-';
    perms[5] = (mode & S_IXGRP)? 'x' : '-';
    perms[6] = (mode & S_IROTH)? 'r' : '-';
    perms[7] = (mode & S_IWOTH)? 'w' : '-';
    perms[8] = (mode & S_IXOTH)? 'x' : '-';
    perms[9] = '\0';
    fprintf(stdout,"THE PERMISSIONS IS: %s.\n",perms);
}
int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr,"USAGE: %s <filepath>.\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    struct stat filestat;
    if(stat(argv[1],&filestat) == -1){
        perror("stat");
        exit(EXIT_FAILURE);
    }
    printFileType(argv[1],filestat.st_mode);
    printf("SIZE: %lld.\n",filestat.st_size);
    printPermission(filestat.st_mode);
}