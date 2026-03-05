#define MODE 0644
#define BUFFER_SIZE 2048

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

int sum(char *filename){
    int fi, size;
    int totalSum = 0;

    if((fi = open(filename,O_RDONLY,MODE)) == -1){
        perror("FILE COULD NOT BE OPENED");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE] = {'\0'};
    char tmp[BUFFER_SIZE] = {'\0'};
    int i = 0;

    while((size = read(fi,buffer,BUFFER_SIZE)) > 0){
        //COSI' SI GESTISCE LA LETTURA DAL FILE RIGA PER RIGA
        //SI RICORDA CHE READ LEGGE UN TOT BYTE ALLA VOLTA
        //QUINDI SE ABBIAMO 15\n6\n7 LI LEGGERA INSIEME
        //È PER QUESTO CHE POI SI DIVIDONO LE RIGHE
        for(int j = 0; j < size; j++){
            if(buffer[j] != '\n'){
                tmp[i++] = buffer[j];//IN UNA VARIABILE TMP SALVIAMO IL CARATTERE PRELEVATO
            }else{ //QUANDO SI INCORRE IN UN CARATTERE \n, SI AGGIUNGE IL CARATTERE NULLO ALLA STRINGA
                //SI CONVERTE
                //E INFINE SI SOMMA
                tmp[i] = '\0';
                totalSum += atoi(tmp);
                i = 0;//SI RIPORTA L'INDICE DELL'ARRAY TEMPORANEO A 0
            }
        }
    }

    if(i > 0){
        //SE L'ULTIMO NUMERO NON HA \n SI INCLUDE LO STESSO
        tmp[i] = '\0';
        totalSum += atoi(tmp);
    }

    if(size == -1){
        perror("ERROR WHILE READING");
        close(fi);
        exit(EXIT_FAILURE);
    }

    close(fi);
    return totalSum;
}




int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stdout,"ERROR, INVALID NUMBER OF ARGUES");
        exit(EXIT_FAILURE);
    }
    

    fprintf(stdout,"THE SUM IS: %d.\n",sum(argv[1]));
}
