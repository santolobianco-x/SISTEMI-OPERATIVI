#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#define DEFAULT_NUM_PROCESSES 2
#define MAX_NUM_PROCESSES 50
#define STRING_LENGTH 10

void func(char *name, int id){
    char str[STRING_LENGTH];

    if(id >= 0){
        sprintf(str,"%s[%d]",name,id);
    }else{
        sprintf(str,"%s",name);
    }
    sleep(1);
    fprintf(stdout, "CIAO SONO %s CON PID = %d\n",str,getpid());
    if(id >= 0){
        exit(id);
    }
    
}



int main(int argc, char **argv){
    int nprocesses = DEFAULT_NUM_PROCESSES;
    if(argc > 1){
        if(((nprocesses = atoi(argv[1]))< 1) || nprocesses > MAX_NUM_PROCESSES){
            fprintf(stderr, "INVALID NUMBER OF GENERABLE FORK\n");
            exit(EXIT_FAILURE);
        }
    }

    func("P",-1);
    
    for(int i = 0; i < nprocesses; i++){
        pid_t r = fork();
        if(r == -1){
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if(r == 0){
            func("F",i);
        }else{
            int status;
            fprintf(stdout,"SONO IL PADRE %d E ATTENDO IL FIGLIO %d\n", getpid(),r);
            wait(&status);
            if(WIFEXITED(status)){
                fprintf(stdout, "EXIT STATUS = %d DEL FIGLIO CON PID = %d\n", WEXITSTATUS(status), r);
            }
        }
    }
}