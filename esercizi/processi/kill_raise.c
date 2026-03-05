#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>



void exit4son(int signal){
    fprintf(stdout, "PROCESS KILLED: %d WITH SIG: %d\n", getpid(), signal);
    exit(EXIT_SUCCESS);
}

void exit4father(int signal){
    fprintf(stdout, "PROCESS KILLED: %d WITH SIG: %d\n", getpid(), signal);
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv){
    // associa al segnale indicato una funzione
    signal(SIGINT,exit4son);
    // non arriverà mai alla funzione associata dato che è un'interruzione brusca
    signal(SIGKILL, exit4father);
    pid_t f;

    if((f = fork()) == 0){
        while(1){
            fprintf(stdout, "SON > MY PID IS: %d\n", getpid());
        }
    } else if(f == -1){
        perror("fork");
        exit(EXIT_FAILURE);
    } else {
        int status;
        sleep(3);
        // terminazione del figlio con codice SIGINT(figlio terminato in modo gentile)
        // 'kill' invia un segnale al processo destinatario
        kill(f,SIGINT);
        wait(&status);
        if(WIFEXITED(status)){
            fprintf(stdout,"FATHER > EXIT STATUS OF MY SON %d\n", WEXITSTATUS(status));
        }

        // terminazione del padre con codice SIGKILL(padre terminato in modo brusco)
        // 'raise' invia un segnale al processo chiamante
        raise(SIGKILL);
    }

    return 0;
}