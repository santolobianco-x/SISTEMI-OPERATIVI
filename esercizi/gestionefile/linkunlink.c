#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define BUFFER_SIZE 2048
#define MODE 0644

int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr,"USAGE: %s <file>",argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd = open(argv[1],O_CREAT | O_TRUNC | O_WRONLY,MODE);
    if(fd == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    strcpy(buffer,"QUESTO È UN FILE DI TEST.\n");

    if(write(fd,buffer,strlen(buffer)) != strlen(buffer)){
        perror("write");
        exit(EXIT_FAILURE);
    }

    //l'hard link corrisponde ad un'ulteriore entry directory agli inode
    // dell'entry originale
    if(link(argv[1],"hardlink.txt") == -1){
        perror("link");
        exit(EXIT_FAILURE);
    }

    //il symbolick link è un puntatore testuale ad un altro percorso
    if(symlink(argv[1],"symlink.txt") == -1){
        perror("symlink");
        exit(EXIT_FAILURE);
    }


    /*LA DIFFERENZA TRA HARDLINK E SYMLINK, SE VIENE CANCELLATO IL FILE DI ORIGINE:
    HARDLINK -> CONTINUA A FUNZIONARE DATO CHE È UN'ULTERIORE ENTRY
    SYMLINK -> IL COLLEGAMENTO SI ROMPE, SMETTE DI FUNZIONARE*/
    
    if(rename(argv[1],"renamed.txt") == -1){
        perror("rename");
        exit(EXIT_FAILURE);
    }

    ssize_t len = readlink("symlink.txt",buffer,sizeof(buffer)-1);

    if(len == -1){
        perror("readlink");
        exit(EXIT_FAILURE);
    }else{
        buffer[len] = '\0';
        fprintf(stdout,"symlink.txt punta a %s\n",buffer);
    }

    if(unlink("hardlink.txt")== -1){
        perror("unlink hardlink.txt");
    }
    //UNLINK(SYSCALL) CANCELLA TUTTI GLI HARD LINK COLLEGATI AL FILE
    //SE VIENE CANCELLATO L'ULTIMO LINK, IL FILE VIENE CANCELLATO
    if(remove("symlink.txt") == -1){
        perror("remove symlink.txt");
    }
    
    if(remove("renamed.txt") == -1){
        perror("remove renamed.txt");
    }
    //REMOVE(<stdio.h>) RICHIAMA unlink SE IL PATH È DI UN FILE, ALTRIMENTI rmdir SE È UNA DIRECTORY.


    if(close(fd) == -1){
        perror("close");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}