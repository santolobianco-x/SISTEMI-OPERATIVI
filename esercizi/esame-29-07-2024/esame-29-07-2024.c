#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define BUFFER_SIZE 100
#define KEYS_SIZE 30

typedef struct {
    char buffer[BUFFER_SIZE];
    int exit;
}shared_data;

typedef struct {
    char key[KEYS_SIZE];
    int index;
    shared_data *data;
    sem_t request;
    sem_t done;
    pthread_t tid;
}thread_data;


void decrypt_locked(char *text, char *key){
    char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int length = strlen(text);
    for(int i = 0; i < length; i++){
        if (text[i] < 'A' || text[i] > 'Z') continue;
        for(int j = 0; j < 26; j++){
            if(text[i] == key[j]){
                text[i] = alphabet[j];
                break;
            }
        }
    }
                        
}


void *thread_function(void *arg){
    thread_data *tdata = (thread_data *)arg;
    fprintf(stdout,"[K%d] chiave assegnata: %s\n", tdata->index, tdata->key);
    while(1){
        sem_wait(&tdata->request);
        if(tdata->data->exit){
            break;
        }
        int lenght = strlen(tdata->data->buffer);
        int idx = tdata->index;
        fprintf(stdout,"[K%d] sto decifrando la frase di %d caratteri passati.\n", idx, lenght);
        decrypt_locked(tdata->data->buffer,tdata->key);
        sem_post(&tdata->done);
    }
    pthread_exit(NULL);
}



int parse_text(char *buffer, char *cifar_text, int *cifar_index){
    char tmp[BUFFER_SIZE];
    strncpy(tmp, buffer, BUFFER_SIZE - 1);
    tmp[BUFFER_SIZE - 1] = '\0';
    char *s1 = strtok(tmp,":\n");
    char *s2 = strtok(NULL, ":\n");
    if(!s1 || !s2){
        fprintf(stderr, "STRINGA NON VALIDA PER LA LETTURA\n");
        return -1;
    }
    *cifar_index = atoi(s1);
    strncpy(cifar_text, s2, strlen(s2));
    cifar_text[strlen(s2)] = '\0';
    return 0;
}

int main(int argc, char **argv){
    if(argc != 4){
        fprintf(stderr,"USO: %s <keys-file> <ciphertext-input-file> [plaintext-output-file]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *keysfile = argv[1];
    char *inputfile = argv[2];
    char *outputfile = argv[3];

    FILE *fpk = fopen(keysfile, "r");
    FILE *fpi = fopen(inputfile, "r");
    FILE *fpo = fopen(outputfile, "w");

    if(!fpk || !fpi || !fpo){
        fprintf(stderr,"ERRORE DURANTE L'APERTURA DEI FILES\n");
        exit(EXIT_FAILURE);
    }
    int n_keys = 0;
    char key[KEYS_SIZE];

    while(fgets(key,sizeof(key),fpk)){
        n_keys++;
    }

    fprintf(stdout,"IL NUMERO DI CHIAVI PRESENTI NEL FILE È %d\n",n_keys);
    fseek(fpk, 0, SEEK_SET);
    if(n_keys < 2){
        fprintf(stderr,"IL NUMERO DI CHIAVI DEVE ESSERE >= 2.\n");
        fclose(fpk);
        fclose(fpo);
        fclose(fpi);
        exit(EXIT_FAILURE);
    }
    
    shared_data *data = (shared_data*) malloc(sizeof(shared_data));
    data->exit = 0;

    thread_data *arrt = (thread_data*) malloc(sizeof(thread_data)*n_keys);
    
    for(int i = 0; i < n_keys; i++){
        if(fgets(key,sizeof(key),fpk) == NULL){
            fprintf(stderr,"ERRORE NELLA LETTURA DELLA CHIAVE\n");
            exit(EXIT_FAILURE);
        }


        int length = strlen(key);
        if(key[length-1] == '\n' || key[length-1] == '\r'){
            key[length-1] = '\0';
        }


        strcpy(arrt[i].key,key);
        arrt[i].index = i;
        arrt[i].data = data;

        if(sem_init(&arrt[i].request, 0, 0) != 0){
            fprintf(stdout, "ERRORE DURANTE LA CREAZIONE DEL SEMAFORO.\n");
            exit(EXIT_FAILURE);
        }

        if(sem_init(&arrt[i].done, 0, 0) != 0){
            fprintf(stdout, "ERRORE DURANTE LA CREAZIONE DEL SEMAFORO.\n");
            exit(EXIT_FAILURE);
        }
    }   
    
    fclose(fpk);



    for(int i = 0; i < n_keys; i++){
        if(pthread_create(&arrt[i].tid, NULL, thread_function, &arrt[i]) != 0){
            fprintf(stderr,"ERRORE NELLA CREAZIONE DEL THREAD.\n");
            exit(EXIT_FAILURE);
        }
    }


    char buffer[BUFFER_SIZE];
    char cifar_text[BUFFER_SIZE];
    int cifar_index;


    while(fgets(buffer,BUFFER_SIZE,fpi)){
        int length = strlen(buffer);
        if(length > 0 &&(buffer[length-1]== '\n' || buffer[length-1] == '\r')){
            buffer[length-1] = '\0';
        }
        if(parse_text(buffer,cifar_text,&cifar_index) ==-1){
            continue;
        }

        if(cifar_index < 0 || cifar_index >= n_keys){
            fprintf(stderr,"INDICE CHIAVE NON VALIDO\n");
            continue;
        }

        fprintf(stdout,"[M] la riga '%s' deve essere decifrata con la chiave n. %d\n", cifar_text, cifar_index);
        strcpy(data->buffer, cifar_text);
        sem_post(&arrt[cifar_index].request);
        sem_wait(&arrt[cifar_index].done);
        fprintf(stdout,"[M] la riga è stata decifrata in: %s\n", data->buffer);
        fprintf(fpo,"%s\n",data->buffer);
    }

    fclose(fpi);
    fclose(fpo);

    data->exit = 1;

    for(int i = 0; i <n_keys; i++){
        sem_post(&arrt[i].request);
        pthread_join(arrt[i].tid, NULL);

        sem_destroy(&arrt[i].request);
        sem_destroy(&arrt[i].done);
    }

    free(data);
    free(arrt);
    exit(EXIT_SUCCESS);
}


