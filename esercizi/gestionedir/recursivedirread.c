#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#define PATH_SIZE 2048

void recursiveread(char *path,int level){
    fprintf(stdout,"READING FROM %s LOCATED INTO THE %d^ LEVEL:\n",path,level);

    DIR *dirpointer = opendir(path);
    if(dirpointer == NULL){
        perror("opendir");
        exit(EXIT_FAILURE);
    }


    struct dirent *dd;
    while((dd = readdir(dirpointer))!= NULL){
        fprintf(stdout,"- %s\n",dd->d_name);
        if(strcmp(".",dd->d_name) != 0 && strcmp("..",dd->d_name) != 0){
            struct stat sf;
            char fullpath[PATH_SIZE];
            snprintf(fullpath,sizeof(fullpath),"%s/%s",path,dd->d_name);
            if(stat(fullpath,&sf) == -1){
                perror("stat");
                exit(EXIT_FAILURE);
            }
            if(S_ISDIR(sf.st_mode)){
                recursiveread(fullpath,level+1);
            }
        }
    }


    if(closedir(dirpointer) == -1){
        perror("closedir");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout,"EXITING FROM %s LOCATED INTO THE %d^ LEVEL:\n",path,level);
}


int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr,"USAGE: %s <path>",argv[0]);
        exit(EXIT_FAILURE);
    }

    recursiveread(argv[1],0);
}