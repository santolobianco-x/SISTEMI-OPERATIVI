#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>



#define MID_QUEUE_SIZE 5
#define END_QUEUE_SIZE 1



typedef struct{
    int length;
    int *arr;
    int id;
    int seq;
    int side;
} queue_arg;


typedef struct{
    int tail, head, count;
    int size;
    pthread_mutex_t mtx;
    pthread_cond_t notfull;
    pthread_cond_t notempty;
    queue_arg *nodes;
} Queue;


typedef struct{
    int num_readers;
    pthread_mutex_t mtx;
} shared_data;

typedef struct{
    Queue *midqueue;
    char *filename;
    char *map;
    int map_length;
    int id;
    int side;
    shared_data *sd;
    pthread_t tid;
} reader_arg;


typedef struct{
    Queue *midqueue;
    Queue *endqueue;
    pthread_t tid;
    shared_data *sd;
} verifier_arg;




int initQueue(Queue *queue, int dim){
    queue->nodes = calloc(dim,sizeof(queue_arg));
    if(!queue->nodes){
        fprintf(stderr,"Errore durante l'allocazione della coda\n");
        return -1;
    }
    
    queue->size = dim;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    int e1,e2;
    e1 = pthread_mutex_init(&queue->mtx, NULL);
    if(e1 != 0){
        fprintf(stderr,"Errore durante la creazione del mutex\n");
        return -1;
    }
    e1 = pthread_cond_init(&queue->notempty, NULL);
    e2 = pthread_cond_init(&queue->notfull, NULL);
    if(e1 != 0 || e2 != 0){
        fprintf(stderr,"Errore durante la creazione delle variabili condizione\n");
        return -1;
    }
    
    return 0;
}

void destroyQueue(Queue *queue){
    free(queue->nodes);
    pthread_cond_destroy(&queue->notempty);
    pthread_cond_destroy(&queue->notfull);
    pthread_mutex_destroy(&queue->mtx);
}



void enqueue(Queue *queue, queue_arg qa){
    pthread_mutex_lock(&queue->mtx);
    while(queue->count >= queue->size){
        pthread_cond_wait(&queue->notfull, &queue->mtx);
    }
    queue->nodes[queue->tail] = qa;
    queue->tail = (queue->tail+1)%queue->size;
    queue->count++;
    pthread_cond_signal(&queue->notempty);
    pthread_mutex_unlock(&queue->mtx);
}

void dequeue(Queue *queue, queue_arg *qa){
    pthread_mutex_lock(&queue->mtx);
    while(queue->count <= 0){
        pthread_cond_wait(&queue->notempty, &queue->mtx);
    }
    *qa = queue->nodes[queue->head];
    queue->head = (queue->head +1)%queue->size;
    queue->count--;
    pthread_cond_signal(&queue->notfull);
    pthread_mutex_unlock(&queue->mtx);
}


int *parseline(int length, char *buffer ){
    int *arr = calloc(length, sizeof(int));
    if(!arr){
        return NULL;
    }
    for(int i = 0; i < length; i++){
        arr[i] = buffer[i];
    }
    return arr;
}

void printmatrix(int side, int *arr){
    for(int i = 0; i < side*side; i+=side){
        fprintf(stdout, "(");
        for(int j = 0; j < side; j++){
            if(j == side-1){
                fprintf(stdout," %d", arr[i+j]);
            }else{
                fprintf(stdout," %d,", arr[i+j]);
            }
        }
        fprintf(stdout,") ");
    }
}


void printmatrixmain(int side, int *arr){
    for(int i = 0; i < side*side; i+=side){
        fprintf(stdout, "(");
        for(int j = 0; j < side; j++){
            if(j == side-1){
                fprintf(stdout," %d", arr[i+j]);
            }else{
                fprintf(stdout," %d,", arr[i+j]);
            }
        }
        fprintf(stdout,")\n");
    }
}



bool is_semi_magic(int side, int *arr){
    int result = 0; 
    for(int i = 0; i < side; i++) result += arr[i];


    for(int i = 0; i < side*side; i+=side){
        int cur_result = 0;
        for(int j= 0; j < side; j++) cur_result+=arr[i+j];

        if(cur_result != result){
            return false;
        }
    }
    
    
    for(int i = 0; i < side; i++){
        int cur_result = 0;
        for(int j= 0; j < side*side; j+=side) cur_result+=arr[i+j];

        if(cur_result != result){
            return false;
        }
    }

    return true;
}

void *thread_reader(void *arg){
    reader_arg *rarg = (reader_arg *) arg;
    int fd = open(rarg->filename, O_RDWR);

    if(fd == -1){
        perror("open");
        fprintf(stdout, "[READER-%d] terminazione\n", rarg->id);
        pthread_mutex_lock(&rarg->sd->mtx);
        rarg->sd->num_readers--;
        pthread_mutex_unlock(&rarg->sd->mtx);
        pthread_exit(NULL);
    }

    

    struct stat st;
    if(fstat(fd,&st) == -1){
        perror("fstat");
        fprintf(stdout, "[READER-%d] terminazione\n", rarg->id);
        pthread_mutex_lock(&rarg->sd->mtx);
        rarg->sd->num_readers--;
        pthread_mutex_unlock(&rarg->sd->mtx);
        pthread_exit(NULL);
    }

    rarg->map_length = st.st_size;
    
    rarg->map = mmap(NULL, rarg->map_length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(rarg->map == MAP_FAILED){
        perror("map");
        fprintf(stdout, "[READER-%d] terminazione\n", rarg->id);
        pthread_mutex_lock(&rarg->sd->mtx);
        rarg->sd->num_readers--;
        pthread_mutex_unlock(&rarg->sd->mtx);
        pthread_exit(NULL);
    }

    fprintf(stdout,"[READER-%d] file %s\n", rarg->id, rarg->filename);

    int buffersize = rarg->side * rarg->side *sizeof(int);
    int seq = 1;

    

    for(int i = 0; i < rarg->map_length; i+= buffersize){
        int *arr = calloc(buffersize, sizeof(int));
        if(!arr){
            fprintf(stderr,"Errore durante l'allocazione\n");
            fprintf(stdout, "[READER-%d] terminazione\n", rarg->id);
            pthread_mutex_lock(&rarg->sd->mtx);
            rarg->sd->num_readers--;
            pthread_mutex_unlock(&rarg->sd->mtx);
            pthread_exit(NULL);
        }
        for(int j = 0; j < buffersize; j++){
            arr[j] = rarg->map[i+j];
        }
        queue_arg q;
        q.arr = arr;
        q.id = rarg->id;
        q.seq = seq;
        q.length = buffersize;
        q.side = rarg->side;
        enqueue(rarg->midqueue, q);
        fprintf(stdout,"[READER-%d] quadrato candidato n.%d: ", q.id, q.seq);
        printmatrix(rarg->side, q.arr);
        puts("");
    }
    fprintf(stdout, "[READER-%d] terminazione\n", rarg->id);
    pthread_mutex_lock(&rarg->sd->mtx);
    rarg->sd->num_readers--;
    pthread_mutex_unlock(&rarg->sd->mtx);
    close(fd);
    msync(rarg->map, rarg->map_length, MS_SYNC);
    munmap(rarg->map, rarg->map_length);
    pthread_exit(NULL);
}


void *thread_verifier(void *arg){
    verifier_arg *varg = (verifier_arg *) arg;
    int n_readers;
    int n_resources;
    while(1){
        pthread_mutex_lock(&varg->sd->mtx);
        n_readers = varg->sd->num_readers;
        pthread_mutex_unlock(&varg->sd->mtx);
        pthread_mutex_lock(&varg->endqueue->mtx);
        n_resources = varg->endqueue->count;
        pthread_mutex_unlock(&varg->endqueue->mtx);


        if(n_readers <= 0 && n_resources <= 0){
            break;
        }

        queue_arg qa;
        dequeue(varg->midqueue, &qa);
        fprintf(stdout,"[VERIF] verifico quadrato: ");
        printmatrix(qa.side,qa.arr);
        puts("");
        if(is_semi_magic(qa.side, qa.arr)){
            fprintf(stdout,"[VERIF] trovato quadrato semi-magico!\n");
            enqueue(varg->endqueue, qa);
        }
    }
    fprintf(stdout,"[VERIF] terminazione\n");
    pthread_exit(NULL);
}





int main(int argc, char **argv){

    if(argc < 3){
        fprintf(stderr,"USO: %s <M-square-size> <bin-file-1> ... <bin-file-N>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int side = atoi(argv[1]);
    if(side <3){
        fprintf(stderr,"Il valore di side deve essere compreso tra 3 e 16");
        exit(EXIT_FAILURE);
    }
    
    int num_readers = argc-2;

    verifier_arg *varg = calloc(1,sizeof(verifier_arg));
    reader_arg *rarg = calloc(num_readers, sizeof(reader_arg));
    shared_data sd;
    

    if(pthread_mutex_init(&sd.mtx, NULL)!= 0){
        fprintf(stderr,"Errore durante la creazione del mutex\n");
        free(varg);
        free(rarg);
        exit(EXIT_FAILURE);
    }
    sd.num_readers = num_readers;

    
    Queue *midqueue = calloc(1, sizeof(Queue));
    Queue *endqueue = calloc(1, sizeof(Queue));

    int e1 = initQueue(midqueue, MID_QUEUE_SIZE);
    int e2 = initQueue(endqueue, END_QUEUE_SIZE);
    
    if(e1 == -1 || e2 == -1){
        fprintf(stderr,"Errore durante la creazione delle code\n");
        free(midqueue);
        free(endqueue);
        free(varg);
        free(rarg);
        pthread_mutex_destroy(&sd.mtx);
        exit(EXIT_FAILURE);
    }

    

    for(int i = 0; i < num_readers; i++){
        rarg[i].filename = argv[i+2];
        rarg[i].id = i+1;
        rarg[i].midqueue = midqueue;
        rarg[i].side = side;
        rarg[i].sd = &sd;
        if(pthread_create(&rarg[i].tid, NULL, thread_reader, &rarg[i])!= 0){
            pthread_mutex_lock(&sd.mtx);
            sd.num_readers--;
            pthread_mutex_unlock(&sd.mtx);
        }
    }

    varg->endqueue = endqueue;
    varg->midqueue = midqueue;
    varg->sd = &sd;
    pthread_create(&varg->tid, NULL, thread_verifier, varg);
    
    int n_resources = 0; 
    int lastreaders = 0;
    int magic_semi_square = 0;
    
    while(1){
        pthread_mutex_lock(&sd.mtx);
        lastreaders = sd.num_readers;
        pthread_mutex_unlock(&sd.mtx);
        pthread_mutex_lock(&endqueue->mtx);
        n_resources = endqueue->count;
        pthread_mutex_unlock(&endqueue->mtx);
        if(lastreaders == 0 && n_resources == 0){
            break;
        }

        queue_arg qa;
        int sum = 0;
        dequeue(endqueue, &qa);
        fprintf(stdout, "[MAIN] quadrato semi-magico trovato:");
        printmatrixmain(qa.side, qa.arr);
        for(int i =0;i < qa.side; i++)sum+=qa.arr[i];
        fprintf(stdout,"totale semi-magico %d\n", sum);
        free(qa.arr);
        magic_semi_square++;
        usleep(5000);
    }


    destroyQueue(midqueue);
    destroyQueue(endqueue);
    free(midqueue);
    free(endqueue);
    free(varg);
    free(rarg);
    pthread_mutex_destroy(&sd.mtx);
    
    fprintf(stdout,"[MAIN] terminazione con %d quadrati semi-magici trovati\n", magic_semi_square);
    exit(EXIT_SUCCESS);
}
