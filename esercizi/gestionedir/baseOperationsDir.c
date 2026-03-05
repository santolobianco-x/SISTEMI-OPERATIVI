#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>


#define PATH_NAME1 "test_dir1"
#define PATH_NAME2 "test_dir2"
#define BUFFER_SIZE 1024
/*-----DISCLAIMER-----
SI UTILIZZA SLEEP, NON PERCHÈ LEGATO ALL'ARGOMENTO, MA PER MOSTRARE PASSO PASSO I CAMBIAMENTI
*/
int main(){
    char buffer[BUFFER_SIZE];
    //CREAZIONE PRIMA CARTELLA
    if(mkdir(PATH_NAME1,0755) == -1){
        perror("mkdir");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout,"%s CREATED WITH SUCCESS.\n",PATH_NAME1);


    //
    sleep(2);
    //


    //CREAZIONE SECONDA CARTELLA
    if(mkdir(PATH_NAME2,0755) == -1){
        perror("mkdir");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout,"%s CREATED WITH SUCCESS.\n",PATH_NAME2);

    //
    sleep(2);
    //
    


    //CARTELLA CORRENTE
    if(getcwd(buffer,sizeof(buffer)) == NULL){
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout,"CURRENT DIRECTORY: %s.\n",buffer);


    //
    sleep(2);
    //




    //CAMBIO CARTELLA
    fprintf(stdout,"CHANGING DIRECTORY...  \n");
    if(chdir(PATH_NAME1) == -1){
        perror("chdir");
        exit(EXIT_FAILURE);
    }

    //
    sleep(2);
    //


    //CARTELLA CORRENTE
    if(getcwd(buffer,sizeof(buffer)) == NULL){
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout,"CURRENT DIRECTORY: %s.\n",buffer);

    //
    sleep(2);
    //




    //TORNA ALLA CARTELLA PADRE
    fprintf(stdout,"CHANGING DIRECTORY...\n");
    if(chdir("..") == -1){
        perror("chdir ..");
        exit(EXIT_FAILURE);
    }

    //
    sleep(2);
    //


    

    //RIMOZIONE DIRECTORY
    if(rmdir(PATH_NAME1)){
        perror("rmdir");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout,"%s REMOVED WITH SUCCESS.\n",PATH_NAME1);

    //
    sleep(2);
    //

    if(rmdir(PATH_NAME2)){
        perror("rmdir");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout,"%s REMOVED WITH SUCCESS.\n",PATH_NAME2);

    exit(EXIT_SUCCESS);
}

/*

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>


#define MODE 0755
#define PATHSIZE 2048

int main(int argc, char **argv){
    int ddescr = mkdir("testdir",MODE);
    if(ddescr == -1){
        perror("mkdir");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout,"testdir CREATED WITH SUCCESS.\n");

    if(chdir("testdir") == -1){
        perror("chdir");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout,"----CHANGIN DIRECTORY----\n");

    sleep(2);
    char path[PATHSIZE];
    if(getcwd(path,PATHSIZE) == NULL){
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout,"CURRENT DIRECTORY: %s\n",path);
    
    char path1[PATHSIZE];
    if(chdir("..") == -1){
        perror("chdir");
        exit(EXIT_FAILURE);
    }

    if(getcwd(path1,PATHSIZE) == NULL){
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout,"CURRENT DIRECTORY: %s\n",path1);

    if(rmdir("testdir") == -1){
        perror("rmdir");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout,"%s REMOVED\n",path);
    
}

*/