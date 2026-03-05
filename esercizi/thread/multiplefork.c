#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>


#define DEFAULT_NUM_CHILDREN 2
#define MAX_NUM_CHILDREN 20
#define NUM_STEPS 100

void workloadfunction(const char* name, int child_num){
    char buffer[10];

    if(child_num > 0){
        snprintf(buffer,sizeof(buffer),"[%s%d]",name,child_num);
    }else{
        snprintf(buffer,sizeof(buffer),"[%s]",name);
    }
    for(int i = 0; i < NUM_STEPS; i++){
        fprintf(stdout,"%s %d\n",buffer,i);
    }
    fprintf(stdout,"%s terminato!\n",buffer);
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv){
    int n_children = DEFAULT_NUM_CHILDREN;
    if(argc > 1){
        if((n_children = atoi(argv[1])) < 1 || n_children > MAX_NUM_CHILDREN ){
            fprintf(stdout,"INVALID NUMBER OF CHILDREN(range [%d,%d])",DEFAULT_NUM_CHILDREN,MAX_NUM_CHILDREN);
            exit(EXIT_FAILURE);
        }
    }

    for(int i = 1; i <= n_children; i++){
        if(fork() == 0){
            workloadfunction("F",i);
        }
    }

    wait(NULL);
    workloadfunction("P",0);
}