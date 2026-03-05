#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h> //funzionante solo su LINUX 
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/dirent.h>
#include <dirent.h>
#include <unistd.h>

#define SHARED_BUFFER_SIZE 10
#define END_BUFFER_SIZE 1
#define PATH_MAX 512
#define FILENAME 256



typedef struct {
    char *filename;
    int size;
} buffer_arg;



typedef struct{
    int num_readers;
    pthread_mutex_t mtx;
} exiting_info;

typedef struct{
    buffer_arg *args;
    pthread_mutex_t mtx;
    int tail, head, size;
    sem_t notfull;
    sem_t notempty;
} buffer;

typedef struct{
    int id;
    pthread_t tid;
    buffer *shared_buffer;
    char *dir_name;
    exiting_info *exinfo;
} reader_dir_arg;

typedef struct {
    pthread_t tid;
    buffer *shared_buffer;
    buffer *end_buffer;
} stat_arg;


int initbuffer(buffer *b, int dim){
    b->size = dim;
    b->head = 0;
    b->tail = 0;
    b->args = calloc(dim, sizeof(buffer_arg));

    if(!b->args){
        fprintf(stderr, "Errore durante la creazione del buffer\n");
        return -1;
    }


    if(pthread_mutex_init(&b->mtx, NULL) != 0){
        fprintf(stderr, "Errore durante la creazione del mutex\n");
        return -1;
    }


    int e1 = sem_init(&b->notempty, 0, 0);
    int e2 = sem_init(&b->notfull, 0, dim);
    if(e1 != 0 || e2 != 0){
        fprintf(stderr, "Errore durante la creazione del semaforo\n");
        return -1;
    }
    return 0;
}

void destroybuffer(buffer *b){
    free(b->args);
    pthread_mutex_destroy(&b->mtx);
    sem_destroy(&b->notempty);
    sem_destroy(&b->notfull);
}


void insert(buffer *b, buffer_arg ba){
    sem_wait(&b->notfull);
    pthread_mutex_lock(&b->mtx);
    b->args[b->tail] = ba;
    b->tail = (b->tail+1)%b->size;
    pthread_mutex_unlock(&b->mtx);
    sem_post(&b->notempty);
}

void take(buffer *b, buffer_arg *ba){
    sem_wait(&b->notempty);
    pthread_mutex_lock(&b->mtx);
    *ba = b->args[b->head];
    b->head = (b->head+1)%b->size;
    pthread_mutex_unlock(&b->mtx);
    sem_post(&b->notfull);
}




void *thread_reader(void *arg){
    reader_dir_arg * rda = (reader_dir_arg *)arg;
    
    struct stat st;
    if(stat(rda->dir_name, &st) == -1){
        perror("stat");
        pthread_exit(NULL);
    }


    
    if(!S_ISDIR(st.st_mode)){
        fprintf(stderr,"[D-%d] il file \'%s\' non è una cartella\n", rda->id, rda->dir_name);
        fprintf(stderr,"[D-%d] terminazione\n", rda->id);
        pthread_exit(NULL);
    }

    

    
    DIR *dirp = opendir(rda->dir_name);
    if(!dirp){
        fprintf(stderr,"[D-%d] errore durante l'apertura della cartella\n", rda->id);
        fprintf(stderr,"[D-%d] terminazione\n", rda->id);
        pthread_exit(NULL);
    }


    
    struct dirent *dirs;
    fprintf(stdout,"[D-%d] scansione della cartella \'%s\'...\n", rda->id, rda->dir_name);
    while((dirs = readdir(dirp))!= NULL){
        char PATH[PATH_MAX];
        
        snprintf(PATH, PATH_MAX,"%s/%s",rda->dir_name, dirs->d_name);
        char *filename = strdup(PATH);
        struct stat stfile;
        if(stat(filename, &stfile) == -1){
            free(filename);
            continue;
        }
        if(!S_ISREG(stfile.st_mode)){
            free(filename);
            continue;
        }else{
            fprintf(stdout,"[D-%d] trovato il file \'%s\' in \'%s\'\n", rda->id, filename ,rda->dir_name);
            buffer_arg ba = {.filename = filename, .size = 0};
            insert(rda->shared_buffer, ba);
        }
    }
    fprintf(stdout,"[D-%d] terminazione\n", rda->id);
    
    int lastreaders;
    pthread_mutex_lock(&rda->exinfo->mtx);
    rda->exinfo->num_readers--;
    lastreaders = rda->exinfo->num_readers;
    pthread_mutex_unlock(&rda->exinfo->mtx);

    if(lastreaders == 0){
        buffer_arg ba = {.filename = NULL, .size = -1};
        insert(rda->shared_buffer, ba);
    }
    pthread_exit(NULL);
}


void *thread_stat(void *arg){
    stat_arg *sarg = (stat_arg *)arg;
    
    while(1){
        buffer_arg ba;
        take(sarg->shared_buffer,&ba);
        if(ba.filename == NULL){
            insert(sarg->end_buffer,ba);
            break;
        }
        struct stat st;
        if(stat(ba.filename, &st) == -1){
            free(ba.filename);
            continue;
        }
        ba.size = st.st_size;
        fprintf(stdout,"[STAT] il file \'%s\' ha dimensione %d byte\n", ba.filename, ba.size);
        insert(sarg->end_buffer,ba);

    }
    fprintf(stdout,"[STAT] terminazione\n");
    pthread_exit(NULL);
}


int main(int argc, char **argv){
    if(argc <2){
        fprintf(stderr,"%s <dir-1> <dir-2> ... <dir-n>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n_readers = argc-1;
    reader_dir_arg *rdas = calloc(n_readers, sizeof(reader_dir_arg));
    stat_arg *sarg = calloc(1,sizeof(stat_arg));
    if(rdas == NULL || sarg == NULL){
        fprintf(stderr,"Errrore durante l'allocazione delle strutture\n");
        exit(EXIT_FAILURE);
    }

    buffer shared_buffer, end_buffer;
    exiting_info exinfo= {.num_readers = n_readers};


    if(pthread_mutex_init(&exinfo.mtx, NULL) != 0){
        fprintf(stderr,"Errore durante la creazione del mutex\n");
        free(rdas);
        free(sarg);
        exit(EXIT_FAILURE);
    }


    if(initbuffer(&shared_buffer, SHARED_BUFFER_SIZE) == -1){
        free(rdas);
        free(sarg);
        fprintf(stderr,"Errrore durante la creazione del buffer\n");
        exit(EXIT_FAILURE);
    }



    if(initbuffer(&end_buffer, END_BUFFER_SIZE) == -1){
        free(rdas);
        free(sarg);
        destroybuffer(&shared_buffer);
        fprintf(stderr,"Errrore durante la creazione del buffer\n");
        exit(EXIT_FAILURE);
    }


    for(int i = 0; i < n_readers; i++){
        rdas[i].dir_name = argv[i+1];
        rdas[i].id = i+1;
        rdas[i].shared_buffer = &shared_buffer;
        rdas[i].exinfo = &exinfo;
        pthread_create(&rdas[i].tid, NULL, thread_reader, &rdas[i]);
    }
    

    sarg->end_buffer = &end_buffer;
    sarg->shared_buffer = &shared_buffer;
    pthread_create(&sarg->tid, NULL, thread_stat, sarg);
    

    long long totalbyte = 0;
    while(1){
        buffer_arg ba;
        take(&end_buffer, &ba);
        if(ba.filename == NULL){
            break;
        }
        totalbyte += ba.size;
        fprintf(stdout, "[MAIN] con il file \'%s\' il totale parziale è di %lld byte\n", ba.filename, totalbyte);
        free(ba.filename);
        usleep(5000);
    }

    for(int i = 0; i < n_readers; i++){
        pthread_join(rdas[i].tid, NULL);
    }

    pthread_join(sarg->tid, NULL);

    

    fprintf(stdout, "[MAIN] il totale finale è di %lld byte\n", totalbyte);
    free(rdas);
    free(sarg);
    destroybuffer(&shared_buffer);
    destroybuffer(&end_buffer);
    exit(EXIT_SUCCESS);
}