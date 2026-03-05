#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#define MODE1 0644
#define MODE2 0400

int main(){
    int fd = open("testchwon1.txt",O_CREAT | O_TRUNC | O_WRONLY,MODE1);
    if(fd == -1){
        perror("open testchwon1.txt");
        exit(EXIT_FAILURE);
    }

    char towrite[] = "FILE DI PROVA. SERVE A VERIFICARE IL CAMBIO DEI PERMESSI";
    if((write(fd,towrite,sizeof(towrite)-1) != sizeof(towrite)-1)){
        perror("write");
        exit(EXIT_FAILURE);
    }


    //VENGONO CAMBIATI I PERMESSI, IN SOLA LETTURA PER IL PROPRIETARIO
    if(chmod("testchwon1.txt",MODE2) != 0){
        perror("chmod");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout,"PERMITS CHANGED IN: r-- --- ---\n");
    
    //SE APERTO DEVE ESSERE CHIUSO IL FILE, ALTRIMENTI NON ENTRANO IN MOTO I PERMESSI DURANTE TUTTO IL PROGRAMMA
    if(close(fd) == -1){
        perror("close");
        exit(EXIT_FAILURE);
    }

    //L'APERTURA FALLIRÀ PERCHÈ POSSIBILE SOLO IN MODALITÀ LETTURA
    fd = open("testchwon1.txt",O_CREAT | O_APPEND | O_WRONLY,MODE1);
    if(fd == -1){
        perror("open testchwon1.txt");
    }

    //A CASCATA FALLIRANNO SCRITTURA E CHIUSURA DEL FILE
    char towrite1[] = "SECONDA SCRITTURA";
    if(write(fd,towrite1,sizeof(towrite1)-1) != sizeof(towrite1)-1){
        perror("write");
    }

    if(close(fd) == -1){
        perror("close");
        exit(EXIT_FAILURE);
    }


    /*
    Cambia proprietario e gruppo
    if (chown(file, 1001, 1001) == -1) {
        perror("chown");
        exit(EXIT_FAILURE);
    } se non si vuole cambiare uno dei due
    basta mettere -1 al posto o del proprietario o del grupo

    
    
    */

    exit(EXIT_SUCCESS);
}
