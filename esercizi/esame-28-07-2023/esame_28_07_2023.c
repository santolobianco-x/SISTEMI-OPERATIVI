#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/dirent.h>
#include <dirent.h>

#define DIR_MAX 256
#define PATH_MAX 512
#define NUM_ADDERS 2



typedef struct{
    long long *arr;
    int count;
    int readers;
    pthread_mutex_t mtx;
    pthread_cond_t readable;
} buffer;

typedef struct{
    buffer *b;
    int id;
    pthread_t tid;
    char dir_name[DIR_MAX];
} dir_arg;

typedef struct{
    buffer *b;
    int id;
    pthread_t tid;
} add_arg;



int initbuffer(buffer *b, int n_readers){
    b->count = 0;
    b->arr = NULL;
    b->readers =  n_readers;
    if(pthread_mutex_init(&b->mtx, NULL) != 0){
        fprintf(stderr,"Errore durante la creazione del semaforo\n");
        return -1;
    }
    if(pthread_cond_init(&b->readable, NULL) != 0){
        fprintf(stderr,"Errore durante la creazione della variabile condizione\n");
        return -1;
    }
    return 0;
}


void destroybuffer(buffer *b){
    free(b->arr);
    pthread_cond_destroy(&b->readable);
    pthread_mutex_destroy(&b->mtx);
}


int insertdim(buffer *b, long long dim){
    pthread_mutex_lock(&b->mtx);
    long long *tmp = realloc(b->arr, sizeof(long long)*(b->count+1));
    if(!tmp){
        pthread_mutex_unlock(&b->mtx);
        return -1;
    }
    tmp[b->count] = dim;
    b->arr = tmp;
    b->count++;
    pthread_cond_signal(&b->readable);
    pthread_mutex_unlock(&b->mtx);
    return 0;
}


int substitute(buffer *b, int minidx, int maxidx){
    long long *tmp = calloc(b->count-1, sizeof(long long));
    if(!tmp){
        return -1;
    }


    int j = 0; 
    for(int i = 0; i < b->count && j < b->count-2; i++){
        if(i == minidx || i == maxidx){
            continue;
        }
        
        tmp[j] = b->arr[i];
        j++;
    }
    tmp[b->count-2] = b->arr[minidx]+b->arr[maxidx];
    free(b->arr);
    b->arr = tmp;
    b->count--;
    return 0;
}





void *thread_dir(void* arg){
    dir_arg *darg = (dir_arg *) arg;
    struct stat st;
    if(stat(darg->dir_name, &st) == -1){
        perror("stat");
        fprintf(stdout, "[DIR-%d] terminazione\n", darg->id);
        pthread_mutex_lock(&darg->b->mtx);
        darg->b->readers--;
        pthread_mutex_unlock(&darg->b->mtx);
        pthread_exit(NULL);
    }

    if(!S_ISDIR(st.st_mode)){
        fprintf(stderr,"Il path passato \'%s\' non è corrisponde ad una cartella.\n", darg->dir_name);
        fprintf(stdout, "[DIR-%d] terminazione\n", darg->id);
        pthread_mutex_lock(&darg->b->mtx);
        darg->b->readers--;
        pthread_mutex_unlock(&darg->b->mtx);
        pthread_exit(NULL);
    }

    DIR *dp = opendir(darg->dir_name);
    if(!dp){
        perror("opendir");
        fprintf(stdout, "[DIR-%d] terminazione\n", darg->id);
        pthread_mutex_lock(&darg->b->mtx);
        darg->b->readers--;
        pthread_mutex_unlock(&darg->b->mtx);
        pthread_exit(NULL);
    }

    fprintf(stdout,"[DIR-%d] scansione della cartella \'%s\'...\n", darg->id, darg->dir_name);

    struct dirent *drnt;
    char path[PATH_MAX];
    while((drnt = readdir(dp)) != NULL){
        snprintf(path, PATH_MAX, "%s/%s", darg->dir_name, drnt->d_name);
        struct stat filest;


        if(stat(path,&filest) == -1){
            continue;
        }
        if(!S_ISREG(filest.st_mode)){
            continue;
        }


        if(insertdim(darg->b, filest.st_size) == -1){
            continue;
        }
        fprintf(stdout,"[DIR-%d]trovato il file \'%s\' di %lld byte; l'insieme ha adesso %d elementi.\n", darg->id, path,
        filest.st_size, darg->b->count);
    }

    fprintf(stdout, "[DIR-%d] terminazione\n", darg->id);
    pthread_mutex_lock(&darg->b->mtx);
    darg->b->readers--;
    pthread_mutex_unlock(&darg->b->mtx);
    closedir(dp);
    pthread_exit(NULL);
}


void *thread_add(void *arg){
    add_arg *adda = (add_arg *) arg;
    buffer *b = adda->b;

    for(;;){
        pthread_mutex_lock(&b->mtx);
        

        while (b->readers > 0 && b->count < 2){
            pthread_cond_wait(&b->readable, &b->mtx);
        }
        
        if(b->readers <= 0 && b->count < 2){
            pthread_cond_signal(&b->readable);
            pthread_mutex_unlock(&b->mtx);
            break;
        }

        long long min = __LONG_LONG_MAX__, max = -__LONG_LONG_MAX__;
        int minidx = -1, maxidx = -1;
        for(int i = 0; i < b->count; i++){
            if(b->arr[i] < min){
                min = b->arr[i];
                minidx = i;
            }

            if(b->arr[i] > max){
                max = b->arr[i];
                maxidx = i;
            }
        }

        if(substitute(b, minidx, maxidx) == -1){
            pthread_mutex_unlock(&b->mtx);
            continue;
        }

        fprintf(stdout,"[ADD-%d] il minimo (%lld) e il massimo (%lld) sono stati sostituiti da %lld" 
            "; l'insieme ha adesso %d elementi\n", adda->id, min, max, min+max, b->count);
        pthread_mutex_unlock(&b->mtx);
    }
    fprintf(stdout,"[ADD-%d] terminazione\n", adda->id);
    pthread_exit(NULL);
}




int main(int argc, char **argv){
    if(argc <2){
        fprintf(stderr,"USO: %s <dir-1> <dir-2> ... <dir-n>", argv[0]);
        exit(EXIT_FAILURE);
    }

    int readers = argc-1;
    

    buffer *b = calloc(1, sizeof(buffer));
    initbuffer(b, readers);

    dir_arg *diras = calloc(readers, sizeof(dir_arg));
    add_arg *addas = calloc(NUM_ADDERS, sizeof(add_arg));

    if(!diras || !addas){
        fprintf(stderr,"Errore durante la creazione delle strutture.\n");
        exit(EXIT_FAILURE);
    }


    for(int i = 0; i < readers; i++){
        memcpy(diras[i].dir_name, argv[i+1], DIR_MAX);
        diras[i].b = b;
        diras[i].id = i+1;
        pthread_create(&diras[i].tid, NULL, thread_dir, &diras[i]);
    }

    for(int i = 0; i < NUM_ADDERS; i++){
        addas[i].b = b;
        addas[i].id = i+1;
        pthread_create(&addas[i].tid, NULL, thread_add, &addas[i]);
    }


    for(int i = 0; i < readers; i++){
        pthread_join(diras[i].tid, NULL);
    }

    for(int i = 0; i < NUM_ADDERS; i++){
        pthread_join(addas[i].tid, NULL);
    }

    fprintf(stdout,"[MAIN] i thread secondari hanno terminato e il totale finale è di %lld byte", b->arr[0]);
    destroybuffer(b);
    free(addas);
    free(diras);
}