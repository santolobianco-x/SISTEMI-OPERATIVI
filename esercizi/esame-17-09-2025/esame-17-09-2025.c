#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h> // funzionante solo su LINUX

#define DIM_ARR 12
#define NUM_VERIFIERS 3
#define MID_QUEUE_SIZE 10
#define END_QUEUE_SIZE 1


typedef struct{
    unsigned char arr[DIM_ARR];
    int id;
    int seq;
} queue_arg;

typedef struct{
    queue_arg *arg;
    int size, tail, head, count;
    sem_t notfull;
    sem_t notempty;
    pthread_mutex_t mtx;
} queue;

typedef struct{
    int num_readers;
    int num_verifiers;
    pthread_mutex_t mtx;
}shared_data;


typedef struct{
    char *filename;
    char *map;
    int maplength;
    queue *midqueue;
    shared_data *sd;
    pthread_t tid;
    int id;
} reader_arg;


typedef struct{
    queue *midqueue;
    queue *endqueue;
    pthread_t tid;
    int id;
    shared_data *sd;
} verifier_arg;

int initqueue(queue *q, int dim){
    q->arg = calloc(dim, sizeof(queue_arg));
    if(!q->arg){
        fprintf(stderr,"Errore durante l'allocazione\n");
        return -1;
    }

    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->size = dim;
    int e1, e2;


    e1 = pthread_mutex_init(&q->mtx, NULL);
    if(e1 != 0){
        fprintf(stderr, "Errore durante la creazione del mutex\n");
        free(q->arg);
        return -1;
    }


    e1 = sem_init(&q->notfull, 0, dim);
    e2 = sem_init(&q->notempty, 0, 0);
    if(e1 != 0 || e2 != 0){
        fprintf(stderr, "Errore durante la creazione del semaforo\n");
        pthread_mutex_destroy(&q->mtx);
        free(q->arg);
        return -1;
    }

    return 0;
}


void destroy(queue *q){
    free(q->arg);
    pthread_mutex_destroy(&q->mtx);
    sem_destroy(&q->notempty);
    sem_destroy(&q->notfull);
}


void enqueue(queue *q, queue_arg qa){
    sem_wait(&q->notfull);
    pthread_mutex_lock(&q->mtx);
    q->arg[q->tail] = qa;
    q->tail = (q->tail+1)%q->size;
    q->count++;
    pthread_mutex_unlock(&q->mtx);
    sem_post(&q->notempty);
}


void dequeue(queue *q, queue_arg *qa){
    sem_wait(&q->notempty);
    pthread_mutex_lock(&q->mtx);
    *qa = q->arg[q->head];
    q->head = (q->head+1) % q->size;
    q->count--;
    pthread_mutex_unlock(&q->mtx);
    sem_post(&q->notfull);
}


void printlist(unsigned char *arr, int length){
    for(int i = 0; i < length; i++){
        if(i == length-1){
            fprintf(stdout," %d\n", arr[i]);
        }else{
            fprintf(stdout," %d,", arr[i]);
        }
    }
}


void *thread_reader(void *arg){
    reader_arg *rarg = (reader_arg *) arg;
    int fd = open(rarg->filename, O_RDWR);



    if(fd == -1){
        fprintf(stderr,"[READER-%d] impossibile aprire il file\n", rarg->id);
        pthread_mutex_lock(&rarg->sd->mtx);
        rarg->sd->num_readers--;
        pthread_mutex_unlock(&rarg->sd->mtx);
        pthread_exit(NULL);
    }



    struct stat st;
    if(fstat(fd,&st) == -1){
        perror("stat");
        pthread_mutex_lock(&rarg->sd->mtx);
        rarg->sd->num_readers--;
        pthread_mutex_unlock(&rarg->sd->mtx);
        close(fd);
        pthread_exit(NULL);
    }

    rarg->maplength = st.st_size;
    rarg->map = mmap(NULL, rarg->maplength, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if(rarg->map == MAP_FAILED){
        perror("mmap");
        pthread_mutex_lock(&rarg->sd->mtx);
        rarg->sd->num_readers--;
        pthread_mutex_unlock(&rarg->sd->mtx);
        close(fd);
        pthread_exit(NULL);
    }
    int seq = 1;

    for(int i = 0; i < rarg->maplength; i+= DIM_ARR){
        queue_arg qa;
        fprintf(stdout, "[READER-%d] vettore candidato n.%d: ", rarg->id, seq);
        for(int j = 0; j < DIM_ARR; j++){
            qa.arr[j] = rarg->map[i+j];
        }
        printlist(qa.arr, DIM_ARR);
        qa.id = rarg->id;
        qa.seq = seq;
        enqueue(rarg->midqueue, qa);
        seq++;
    }

    bool islast = false;
    pthread_mutex_lock(&rarg->sd->mtx);
    rarg->sd->num_readers--;
    islast = (rarg->sd->num_readers == 0) ? true : false;
    pthread_mutex_unlock(&rarg->sd->mtx);
    close(fd);
    seq--;
    fprintf(stdout,"[READER-%d] terminazione con %d vettori letti\n", rarg->id, seq);
    munmap(rarg->map, rarg->maplength);
    if(islast){
        for(int i = 0; i < NUM_VERIFIERS; i++){
            queue_arg poison = {.id =-1};
            enqueue(rarg->midqueue,poison);
        }

    }
    pthread_exit(NULL);
}



void *thread_verifier(void *arg){
    verifier_arg *varg = (verifier_arg *)arg;
    int n_resources = 0, n_readers =0;
    

    int seq = 0;
    int oddsum = 0, evensum = 0;
    while(1){
        queue_arg qa;
        dequeue(varg->midqueue,&qa);
        if(qa.id == -1){
            queue_arg poison = {.id = -1};
            enqueue(varg->endqueue, poison);
            break;
        }
        fprintf(stdout,"[VERIF-%d] vettore candidato:", varg->id);
        printlist(qa.arr, DIM_ARR);
        for(int i = 0; i < DIM_ARR; i++){
            if(i%2==0){
                evensum+=qa.arr[i];
            }else{
                oddsum+=qa.arr[i];
            }
        }
        if(evensum == oddsum){
            fprintf(stdout,"[VERIF-%d] si tratta di un vettore equisomma con somma %d!\n", varg->id, oddsum);
            enqueue(varg->endqueue,qa);
        }else{
            fprintf(stdout,"[VERIF-%d] non è un vettore equisomma (somma pari %d vs. dispari %d)\n", varg->id, evensum, oddsum);
        }
        evensum = 0;
        oddsum = 0;
        seq++;
    }

    fprintf(stdout,"[VERIF-%d] terminazione con %d vettori verificati\n", varg->id, seq);
    pthread_exit(NULL);
}



int main(int argc, char **argv){
    if(argc < 2){
        fprintf(stderr,"USO: %s <file-bin-1> <file-bin-2> ... <file-bin-N>\n", argv[0]);
        exit(EXIT_FAILURE);
    }


    int n_readers = argc-1;
    reader_arg *rargs = calloc(n_readers, sizeof(reader_arg));
    verifier_arg *vargs = calloc(3, sizeof(verifier_arg));


    if(!rargs || !vargs){
        fprintf(stderr,"Errore durante l'allocazione delle strutture\n");
    }


    shared_data sd ={.num_readers = n_readers, .num_verifiers = NUM_VERIFIERS};


    if(pthread_mutex_init(&sd.mtx, NULL) != 0){
        fprintf(stderr,"Errore durante la creazione del mutex\n");
        free(rargs);
        free(vargs);
        exit(EXIT_FAILURE);
    }

    queue *midqueue = calloc(1, sizeof(queue));
    queue *endqueue = calloc(1,sizeof(queue));
    if(initqueue(endqueue, END_QUEUE_SIZE) == -1){
        fprintf(stderr,"Errore durante la creazione della coda\n");
        free(vargs);
        free(rargs);
        pthread_mutex_destroy(&sd.mtx);
        exit(EXIT_FAILURE);
    }


    if(initqueue(midqueue, MID_QUEUE_SIZE) == -1){
        fprintf(stderr,"Errore durante la creazione della coda\n");
        free(vargs);
        free(rargs);
        pthread_mutex_destroy(&sd.mtx);
        destroy(endqueue);
        exit(EXIT_FAILURE);
    }




    for(int i = 0; i < n_readers; i++){
        rargs[i].filename = argv[i+1];
        rargs[i].id = i+1;
        rargs[i].midqueue = midqueue;
        rargs[i].sd = &sd;
        pthread_create(&rargs[i].tid, NULL, thread_reader, &rargs[i]);
    }

    for(int i =0; i < NUM_VERIFIERS; i++){
        vargs[i].id = i+1;
        vargs[i].endqueue = endqueue;
        vargs[i].midqueue = midqueue;
        vargs[i].sd = &sd;
        pthread_create(&vargs[i].tid, NULL, thread_verifier, &vargs[i]);
    }

    int counter = 0;
    int poison_counter =0; 
    while(poison_counter < NUM_VERIFIERS){
        queue_arg qa;
        dequeue(endqueue, &qa);
        if(qa.id ==-1){
            poison_counter++;
            continue;
        }
        fprintf(stdout, "[MAIN] ricevuto nuovo vettore equisomma:");
        printlist(qa.arr, DIM_ARR);
        counter++;
        usleep(50000);
    }

    

    for(int i = 0; i < n_readers; i++){
        pthread_join(rargs[i].tid, NULL);
    }

    for(int i =0; i < NUM_VERIFIERS; i++){
        pthread_join(vargs[i].tid, NULL);
    }

    fprintf(stdout,"[MAIN] terminazione con %d vettori equisomma trovati\n", counter);
    destroy(endqueue);
    destroy(midqueue);
    free(endqueue);
    free(midqueue);
    free(vargs);
    free(rargs);
    pthread_mutex_destroy(&sd.mtx);
    exit(EXIT_SUCCESS);
}