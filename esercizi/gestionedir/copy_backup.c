#define MODE 0644
#define BUFFER_SIZE 64
#define BUFFER_SIZE_BACKUP 128


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>



void copy(int fsource, int fdest, int offset){
    lseek(fsource,offset,SEEK_SET);

    int size = 0;
    char buffer[BUFFER_SIZE];

    while((size = read(fsource,buffer,BUFFER_SIZE)) > 0){
        if(size != write(fdest,buffer,size)){
            perror("ERROR WHILE WRITING");
            exit(EXIT_FAILURE);
        }
    }

    if(size == -1){
        perror("ERROR WHILE READING");
        exit(EXIT_FAILURE);
    }

    lseek(fsource,0,SEEK_SET);
    fprintf(stdout,"COPY WITH SUCCESS\n");
}

void backup(int fsource, int fbackup){
    int size = 0;
    char buffer[BUFFER_SIZE_BACKUP];
    int cursorSource = 0;
    int coursorBackup = 0;
    while((size = pread(fsource,buffer,BUFFER_SIZE_BACKUP,cursorSource)) > 0){
        cursorSource += size;
        if(size != pwrite(fbackup,buffer,size,coursorBackup)){
            perror("ERROR WHILE WRITING");
            exit(EXIT_FAILURE);
        }
        coursorBackup += size;
    }
    
    if(size == -1){
        perror("ERROR WHILE READING");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout,"BACKUP WITH SUCCESS\n");
}


int main(int argc, char **argv){
    if(argc != 4){
        fprintf(stderr,"USAGE: %s file_input file_output file_backup.\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int fin, fout, fbackup;
    fin = open(argv[1],O_RDONLY,MODE);
    fout = open(argv[2],O_CREAT | O_TRUNC | O_RDWR, MODE);
    if(fin == -1 || fout == -1){
        perror("FILE COULD NOT BE OPENED");
        exit(EXIT_FAILURE);
    }

    int offset = lseek(fin,0,SEEK_END);
    if(offset == -1){
        perror("ERROR WHILE MOVING CURSOR");
        exit(EXIT_FAILURE);
    }
    offset = offset/2;
    lseek(fin,0,SEEK_SET);

    copy(fin,fout,offset);

    int dupfout = dup(fout);
    if(dupfout < 0){
        perror("ERROR WHILE DUPLICATING");
        exit(EXIT_FAILURE);
    }
    close(fout);

    lseek(dupfout,0,SEEK_END);
    char end[] = "--- Copia completata ---";
    write(dupfout,end,strlen(end));
    fsync(dupfout);

    fbackup = open(argv[3],O_CREAT | O_TRUNC | O_WRONLY,MODE);
    if(fbackup == -1){
        perror("FILE COULD NOT BE OPENED");
        exit(EXIT_FAILURE);
    }

    backup(dupfout,fbackup);
    close(fin);
    close(dupfout);
    close(fbackup);
    exit(EXIT_SUCCESS);
}