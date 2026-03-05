#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFER_LENGTH 1024
#define MAX_ARGUMENTS 20
#define PROMPT "nano-shell > "
#define SEPARATOR " "


int main(int argc, char **argv){
    char command[BUFFER_LENGTH];
    char *args[MAX_ARGUMENTS] = {NULL};
    char *token;

    int len, j;

    while(1){
        fprintf(stdout, "%s", PROMPT);
        fgets(command,BUFFER_LENGTH,stdin);

        len = strlen(command);
        if(command[len-1] == '\n'){
            command[len-1] = '\0';
        }

        if(strlen(command) == 0){
            continue;
        }

        if(strcmp(command,"exit") == 0){
            break;
        }

        token = strtok(command,SEPARATOR);
        if(token == NULL){
            continue;
        }
        j = 0;


        args[j++] = strdup(token);
        while((token = strtok(NULL,SEPARATOR)) != NULL){
            args[j++] = strdup(token);
        }

        for(int i = 0; i < MAX_ARGUMENTS; i++){
            fprintf(stdout,"args[%d] = \"%s\"\n",i,args[i]);
            if(!args[i]){
                break;
            }
        }
        fprintf(stdout, "\n");

        pid_t f;
        if((f = fork()) == -1){
            perror("fork");
            exit(EXIT_FAILURE);
        }else if(f == 0){
            args[j] = NULL;
            if(execvp(args[0],args) == -1){
                perror("exec");
                exit(EXIT_FAILURE);
            }
        }else{
            int status;
            wait(&status);
            int exit_code = WEXITSTATUS(status);
            if(exit_code > 0){
                fprintf(stdout, "EXIT FAILURE WITH exit_code = %d\n", exit_code);

            }

        }


        for(int i = 0; i < MAX_ARGUMENTS; i++){
            if(args[i]){
                free(args[i]);
            }
            args[i] = NULL;
        }


    }
    exit(EXIT_SUCCESS);
}