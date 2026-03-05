#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <unistd.h>


int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr,"USAGE: %s <path>\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    DIR* dirpointer = opendir(argv[1]);
    if(dirpointer == NULL){
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    struct dirent *dd;
    for(int i = 0; i <3 ; i++){
        if((dd = readdir(dirpointer)) == NULL){
            fprintf(stdout,"THERE ARE NO MORE FILE TO READ\n");
            exit(EXIT_SUCCESS);
        }
        fprintf(stdout,"- %s\n",dd->d_name);
    }
    long thirdposition;

    if((thirdposition =telldir(dirpointer)) < 0){
        perror("telldir");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout,"%s%s%s","\n----------------------------\n",
        "POSITION SAVED","\n----------------------------\n");

    for(int i = 0; i < 2; i++){
        if((dd = readdir(dirpointer)) == NULL){
            break;
        }
        fprintf(stdout,"- %s\n",dd->d_name);
    }
    
    seekdir(dirpointer,thirdposition);
    fprintf(stdout,"%s%s%s","\n----------------------------\n",
        "RESTART FROM THIRD POSITION","\n----------------------------\n");

    
    while((dd = readdir(dirpointer)) != NULL){
        fprintf(stdout,"- %s\n",dd->d_name);
    }

    if(closedir(dirpointer) == -1){
        perror("closedir");
        exit(EXIT_FAILURE);
    }
}