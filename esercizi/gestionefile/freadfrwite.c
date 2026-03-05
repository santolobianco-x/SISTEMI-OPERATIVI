#define MODE 0644
#define BUFFER_SIZE 2048
#define NUMOBJECT 5


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>



typedef struct{
    char name[50];
    int grade;
} Student;



int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr,"USAGE: %s namefile\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    Student students[NUMOBJECT];
    fprintf(stdout,"ENTER STUDENTS: name \\n grade\n");
    for(int i = 0; i < NUMOBJECT; i++){
        fprintf(stdout,"ENTER NAME: ");
        fscanf(stdin,"%s",students[i].name);
        fprintf(stdout,"ENTER GRADE: ");
        scanf("%d",&students[i].grade);
    }

    FILE *fp = fopen(argv[1],"w");
    if(fp == NULL){
        perror("FILE COULD NOT BE OPENED");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < NUMOBJECT; i++){
        //FWRITE RITORNA IL NUMERO DI ITEM SCRITTI
        //ERGO IN UNA SOLA LETTURA POSSIAMO SCRIVERE PIU' ELEMENTI
        //AD ESEMPIO SCRIVERE TUTTI GLI ELEMENTI DELL'ARRAY
        if(fwrite(&students[i],sizeof(Student),1,fp) != 1){
            perror("ERROR WHILE WRITING");
            exit(EXIT_FAILURE);
        }
    }
    fprintf(stdout,"WRITTED WITH SUCCESS\n");
    fclose(fp);

    fp = fopen(argv[1],"r");
    if(fp == NULL){
        perror("FILE COULD NOT BE OPENED");
        exit(EXIT_FAILURE);
    }

    Student s;
    for(int i = 0; i < NUMOBJECT; i++){
        if(fread(&s,sizeof(s),1,fp)!= 1){
            perror("ERROR WHILE READING");
            exit(EXIT_FAILURE);
        }
        fprintf(stdout,"NAME: %s GRADE: %d\n",s.name,s.grade);
    }

    fprintf(stdout,"READED WITH SUCCESS\n");
    fclose(fp);
    fp = NULL;
}



