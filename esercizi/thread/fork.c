#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


int main(){
    pid_t pid;
    fprintf(stdout,"Sono il processo %d, mio padre è %d.\n",getpid(),getppid());
    pid = fork();

    if(pid == -1){
        perror("fork");
    }else if(pid == 0){
        fprintf(stdout,"Sono il processo figlio %d, mio padre è %d.\n",getpid(), getppid());
    }else{
        //su MACOS il processo padre viene schedulato prima, quindi, viene eseguito e terminato
        //per questo quando il figlio poi viene eseguito, al posto di stampare il pid del padre
        //stampa pid = 1. Esso infatti nel frattempo è diventato orfano ed è stato addottato
        //dal processo launchd
        fprintf(stdout,"Sono il processo padre %d, ho creato il figlio %d.\n",getpid(),pid);
        //wait(NULL); //se mettissimo un wait, il padre aspetterebbe l'esecuzione del figlio
    }
}