///FINISH, PURE FUNZIONANTE!!!!!!!!!!!!!
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdbool.h>



#define NUM_ELEMENTS 9
#define QUEUE_MID_LENGTH 10
#define QUEUE_END_LENGTH 3
#define BUFFER_SIZE 256


typedef struct {
    int elements[NUM_ELEMENTS];
    int id;
    int seq;
} Queue_arg;


typedef struct{
    Queue_arg *q_arg;
    int head, tail, count;
    pthread_mutex_t mtx;
    pthread_cond_t notempty;
    pthread_cond_t notfull;
    int size;
} Queue;


typedef struct{
    pthread_mutex_t mtx;
    int num_readers;
} shared_memory_main;


typedef struct{
    pthread_t tid;
    int id;
    Queue *midqueue;
    char *path;
    shared_memory_main *smm;
} reader_arg;


typedef struct{
    pthread_t tid;
    int id;
    Queue *midqueue;
    Queue *endqueue;
    shared_memory_main *smm;
} verifier_arg;


int init(Queue *q, int n_elements){
    q->q_arg = calloc(n_elements, sizeof(Queue_arg));
    if(!q->q_arg){
        perror("calloc");
        return -1;
    }

    if(pthread_mutex_init(&q->mtx, NULL) != 0){
        fprintf(stderr, "Errore durante la creazione del mutex\n");
        return -1;
    }

    int c1 = pthread_cond_init(&q->notempty, NULL);
    int c2 = pthread_cond_init(&q->notfull, NULL);
    if(c1 != 0 || c2!= 0){
        fprintf(stderr, "Errore durante la creazione delle varibili condizione\n");
        return -1;
    }
    q->size = n_elements;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    return 0;
}

void destroy(Queue *q){
    free(q->q_arg);
    pthread_mutex_destroy(&q->mtx);
    pthread_cond_destroy(&q->notempty);
    pthread_cond_destroy(&q->notfull);
}


void enqueue(Queue *q, Queue_arg qa){
    pthread_mutex_lock(&q->mtx);
    while(q->count >= q->size){
        pthread_cond_wait(&q->notfull, &q->mtx);
    }
    q->q_arg[q->tail] = qa;
    q->tail = (q->tail + 1)%q->size;
    q->count++;
    pthread_cond_signal(&q->notempty);
    pthread_mutex_unlock(&q->mtx);
}


void dequeue(Queue *q, Queue_arg *qa){
    pthread_mutex_lock(&q->mtx);
    while(q->count <= 0){
        pthread_cond_wait(&q->notempty, &q->mtx);
    }
    *qa = q->q_arg[q->head];
    q->head = (q->head + 1)% q->size;
    q->count--;
    pthread_cond_signal(&q->notfull);
    pthread_mutex_unlock(&q->mtx);
}

void printmatrix(int *arr){
    fprintf(stdout,"(%d, %d, %d) (%d, %d, %d) (%d, %d, %d)\n",
        arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6],
        arr[7], arr[8]);
}

bool ismagic(int *arr, int *tot){
    *tot = arr[0] + arr[1] + arr[2];


    for(int i = 0; i < 9; i += 3){
        int currtot = arr[i] + arr[i+1] + arr[i+2];
        if(currtot != *tot) return false;
    }

    for(int i = 0; i < 3; i++){
        int currtot = arr[i] + arr[i+3] + arr[i+6];
        if(currtot != *tot) return false;
    }

    int result = arr[0] + arr[4]+ arr[8];
    if(result != *tot) return false;
    result = arr[2] + arr[4] + arr[6];
    if(result != *tot) return false;
    return true;
}

int parse_buffer(char *str, int *arr){
    if (!str) return -1;

    if (str[strlen(str)-1] == '\n' || str[strlen(str)-1] == '\r')
        str[strlen(str)-1] = '\0';

    char *token = strtok(str, ",");
    int i;

    for (i = 0; i < NUM_ELEMENTS; i++) {
        if (!token) return -1;   // meno di 9 numeri
        arr[i] = atoi(token);
        token = strtok(NULL, ",");
    }

    if (token != NULL) return -1; // più di 9 numeri

    return 0;
}



void *thread_reader(void *arg){
    reader_arg *rarg = (reader_arg *) arg;
    Queue *queue = rarg->midqueue;
    FILE *fp = fopen(rarg->path, "r");
    if(!fp){
        pthread_mutex_lock(&rarg->smm->mtx);
        rarg->smm->num_readers--;
        pthread_mutex_unlock(&rarg->smm->mtx);
        fprintf(stderr,"[READER-%d] impossibile aprire il file.\n", rarg->id);
        pthread_exit(NULL);
    }
    fprintf(stderr,"[READER-%d] file \'%s\'\n", rarg->id, rarg->path);

    char buffer[BUFFER_SIZE];
    int seq = 1;
    Queue_arg q;
    while(fgets(buffer, sizeof(buffer), fp) != NULL){
        q.id = rarg->id;
        q.seq = seq;
        if(parse_buffer(buffer,q.elements) == -1){
            fprintf(stderr,"[READER-%d] Errore durante la lettura dal file\n", rarg->id);
            break;
        }
        enqueue(queue, q);
        fprintf(stdout,"[READER-%d] quadrato candidato n.%d: ", q.id, q.seq);
        printmatrix(q.elements);
        seq++;
    }
    fprintf(stdout,"[READER-%d] terminazione\n", rarg->id);
    pthread_mutex_lock(&rarg->smm->mtx);
    rarg->smm->num_readers--;
    pthread_mutex_unlock(&rarg->smm->mtx);
    fclose(fp);
    pthread_exit(NULL);
}




void *thread_verifier(void *arg){
    verifier_arg * varg = (verifier_arg *) arg;
    Queue *midq = varg->midqueue;
    Queue *endq = varg->endqueue;
    int n_resources = 0;
    while(1){


        pthread_mutex_lock(&varg->smm->mtx);
        if(varg->smm->num_readers <= 0){
            pthread_mutex_lock(&midq->mtx);
            n_resources = midq->count;
            pthread_mutex_unlock(&midq->mtx);
            if(n_resources <= 0){
                pthread_mutex_unlock(&varg->smm->mtx);
                fprintf(stderr,"[VERIF-%d] terminazione\n", varg->id);
                break;
            }
        }
        pthread_mutex_unlock(&varg->smm->mtx);


        
        Queue_arg q_arg;
        dequeue(midq,&q_arg);

        fprintf(stdout,"[VERIF-%d] verifico quadrato: ", varg->id);
        printmatrix(q_arg.elements);
        int result = 0;
        if(ismagic(q_arg.elements, &result)){
            fprintf(stdout,"[VERIF-%d] trovato quadrato magico\n", varg->id);
            enqueue(endq, q_arg);
        }
    }
    pthread_exit(NULL);
}





int main(int argc, char **argv){
    if(argc < 3){
        fprintf(stderr,"%s <M-verifiers> <file-1> ... <file-N>\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    int m_verifiers = atoi(argv[1]);
    
    if(m_verifiers < 1){
        fprintf(stderr,"M-verifier necessariamente maggiore di 0\n");
    }
    
    
    int n_readers = argc - 2;


    
    reader_arg * rargs = (reader_arg *) calloc(n_readers, sizeof(reader_arg));
    verifier_arg * vargs = (verifier_arg *) calloc(m_verifiers, sizeof(verifier_arg));
    Queue midq, endq;
    init(&midq,QUEUE_MID_LENGTH);
    init(&endq,QUEUE_END_LENGTH);

    pthread_mutex_t mtx;
    pthread_mutex_init(&mtx, NULL);
    shared_memory_main smm = {.mtx = mtx, .num_readers = n_readers};


    for(int i = 0; i < n_readers; i++){
        rargs[i].id = i+1;
        rargs[i].path = argv[i+2];
        rargs[i].midqueue = &midq;
        rargs[i].smm = &smm;
        pthread_create(&rargs[i].tid, NULL, thread_reader, &rargs[i]);
    }

    for(int i = 0; i < m_verifiers; i++){
        vargs[i].endqueue = &endq;
        vargs[i].midqueue = &midq;
        vargs[i].id = i+1;
        vargs[i].smm = &smm;
        pthread_create(&vargs[i].tid, NULL, thread_verifier, &vargs[i]);
    }


    int n_resources = 0;

    while(1){
        pthread_mutex_lock(&smm.mtx);
        n_readers = smm.num_readers;
        pthread_mutex_unlock(&smm.mtx);
        pthread_mutex_lock(&endq.mtx);
        n_resources = endq.count;
        pthread_mutex_unlock(&endq.mtx);
        if(n_readers <= 0 && n_resources <= 0){
            break;
        }
        Queue_arg qa;
        dequeue(&endq, &qa);
        fprintf(stdout, "[MAIN] quadrato magico trovato:");
        printmatrix(qa.elements);
        fprintf(stdout,"totale %d\n", qa.elements[0]+qa.elements[1]+qa.elements[2]);
        usleep(5000);
    }

    for(int i = 0; i < n_readers; i++){
        pthread_join(rargs[i].tid, NULL);
    }


    for(int i = 0; i < m_verifiers; i++){
        pthread_join(vargs[i].tid, NULL);
    }

    destroy(&endq);
    destroy(&midq);

    free(vargs);
    free(rargs);
    fprintf(stdout,"[MAIN] terminazione\n");
}