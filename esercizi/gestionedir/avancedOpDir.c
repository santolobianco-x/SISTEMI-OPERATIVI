#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>


int main(){
    DIR *dc;
    struct dirent *entry;
    //PUNTATORE ALLA CARTELLA CHE CONTIENE IL FILE
    dc = opendir(".");
    if(dc == NULL){
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout,"CONTENTS OF CURRENT FOLDER");


    while((entry = readdir(dc)) != NULL){
        fprintf(stdout,"- %s\n",entry->d_name);
    }


    rewinddir(dc);
    if((entry = readdir(dc)) != NULL){
        fprintf(stdout,"FIRST FILE %s\n",entry->d_name);
    }

    closedir(dc);
}

