#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#define PROPOSALS_MAX 20
#define DISCARDED_MAX 20
#define VERIFIERS 3
#define REPAREIRS 3

typedef struct{
    int *vettore;
    int round;
} queue_arg;


typedef struct{
    pthread_mutex_t mtx;
    sem_t empty;
    sem_t full;
    queue_arg *args;
    int count, tail, head;
    int size;
} queue;



typedef struct{
    const int N;
    const int M;
    pthread_mutex_t mtx;
    int accepted;
    int n_verifiers;
} shared_data;




typedef struct{
    shared_data *s;
    queue *q;
    pthread_t tid;
} generator_arg;



typedef struct{
    queue *proposals;
    queue *discarded;
    shared_data *s;
    pthread_t tid;
    int id;
} verificator_arg;


typedef struct {
    queue *proposals;
    queue *discarded;
    shared_data *s;
    int id;
    pthread_t tid;   
} repairer_arg;



int initqueue(queue *q, int size){
    q->args = (queue_arg *)calloc(size, sizeof(queue_arg));
    
    if(!q->args){
        fprintf(stderr,"Errore durante l'allocazione della coda\n");
        return -1;
    }

    
    q->count = 0;
    q->tail = 0;
    q->head = 0;
    q->size = size;
    
    int e1 = sem_init(&q->empty, 0, size);
    int e2 = sem_init(&q->full, 0, 0);
    if(e1 != 0|| e2 != 0){
        fprintf(stderr,"Errore durante la creazione dei semafori\n");
        return -1;
    }

    

    if(pthread_mutex_init(&q->mtx, NULL) != 0){
        fprintf(stderr,"Errore durante la creazione del mutex\n");
        sem_destroy(&q->full);
        sem_destroy(&q->empty);
        return -1;
    }

    return 0;
}

void destroyqueue(queue *q){
    free(q->args);
    pthread_mutex_destroy(&q->mtx);
    sem_destroy(&q->full);
    sem_destroy(&q->empty);
}

void insert(queue *q, queue_arg qnode){
    sem_wait(&q->empty);
    pthread_mutex_lock(&q->mtx);
    q->args[q->tail] = qnode;
    q->tail = (q->tail + 1) % q->size;
    q->count++;
    pthread_mutex_unlock(&q->mtx);
    sem_post(&q->full);
}


void take(queue* q, queue_arg *qnode){
   sem_wait(&q->full);
   pthread_mutex_lock(&q->mtx);
   *qnode = q->args[q->head];
   q->head = (q->head + 1)%q->size;
   q->count--;
   pthread_mutex_unlock(&q->mtx);
   sem_post(&q->empty);
}

void printvector(int *arr, int dim){
    for(int i = 0; i < dim; i++){
        if(i == dim-1){
            fprintf(stdout," %d\n", arr[i]);
        }else{
            fprintf(stdout," %d,", arr[i]);
        }
    }
}




bool verify(int *arr, int dim){
    int half = dim/2;
    int firstsum = 0;
    int secondsum = 0;
    for(int i = 0; i < half; i++){
        firstsum += arr[i];
    }

    for(int i = half; i < dim; i++){
        secondsum += arr[i];
    }

    return firstsum == secondsum;
}


void *thread_generator(void *arg){
    generator_arg *ga = (generator_arg *)arg;
    shared_data *s = ga->s;
    for(int i = 0; i < s->N; i++){
        int *arr = calloc(s->M, sizeof(int));
        for(int j = 0; j < s->M; j++){
            arr[j] = rand()%100;
        }
        queue_arg qnode = {.vettore = arr, .round = 0};
        insert(ga->q, qnode);
        flockfile(stdout);
        fprintf(stdout,"[GEN] vettore candidato n.%d:", i);
        printvector(arr, s->M);
        funlockfile(stdout);
        usleep(50000);
    }
    fprintf(stdout,"[GEN] terminato\n");
    pthread_exit(NULL);
}




void *thread_verificator(void *arg){
    verificator_arg *va = (verificator_arg *)arg;
    shared_data *s = va->s;


    while(1){
        pthread_mutex_lock(&s->mtx);
        if(s->accepted == s->N){
            for(int i = 0; i < VERIFIERS - 1; i++){  
                queue_arg sentinel = {.vettore = NULL, .round = -1};
                insert(va->proposals, sentinel);
            }
            for(int i = 0; i < REPAREIRS; i++){
                queue_arg sentinel = {.vettore = NULL, .round = -1};
                insert(va->discarded, sentinel);
            }
            pthread_mutex_unlock(&s->mtx);
            break;
        }
        pthread_mutex_unlock(&s->mtx);



        queue_arg qnode;
        take(va->proposals, &qnode);
        if(qnode.round == -1)break;
        flockfile(stdout);
        fprintf(stdout,"[VER-%d] estratto un vettore candidato con round pari a %d:", va->id, qnode.round);
        printvector(qnode.vettore, s->M);


        if(verify(qnode.vettore, s->M)){
            pthread_mutex_lock(&s->mtx);
            s->accepted++;
            pthread_mutex_unlock(&s->mtx);
            fprintf(stdout,"[VER-%d] vettore verificato e accettato dopo %d round di aggiustamenti (%d di %d)\n",
            va->id, qnode.round, s->accepted, s->N);
            free(qnode.vettore);
        }else{
            fprintf(stdout,"[VER-%d] vettore non verificato e rigettato\n", va->id);
            insert(va->discarded, qnode);
        }


        funlockfile(stdout);
        usleep(50000);
        
    }
    fprintf(stdout,"[VER-%d] terminato\n", va->id);
    pthread_exit(NULL);
}


void *thread_repairer(void *arg){
    repairer_arg * ra = (repairer_arg *)arg;
    shared_data *s = ra->s;
    int naccepted = 0;
    int wanted = s->N;
    while(1){
        queue_arg qnode;
        take(ra->discarded, &qnode);
        if(qnode.round == -1){
            break;
        }
        flockfile(stdout);

        fprintf(stdout,"[REP-%d] estratto un vettore da riparare:", ra->id);
        printvector(qnode.vettore, s->M);

        int rndidx = rand()%s->M;
        int before = qnode.vettore[rndidx];
        qnode.vettore[rndidx] = rand()%100;
        qnode.round++;

        fprintf(stdout,"[REP-%d] reinserito vettore riparato (%d->%d) con round pari a %d:", ra->id, before, qnode.vettore[rndidx], qnode.round);
        printvector(qnode.vettore, s->M);
        insert(ra->proposals, qnode);
        funlockfile(stdout);
        usleep(50000);
    }
    fprintf(stdout,"[REP-%d] terminato\n", ra->id);
    pthread_exit(NULL);
}

int main(int argc, char **argv){
    if(argc < 3){
        fprintf(stderr,"USO: %s <N> <M>\n", argv[0]);
        exit(EXIT_FAILURE);
    }


    int n_vectors = atoi(argv[1]);
    int dim = atoi(argv[2]);

    if(n_vectors < 0 || dim == 0){
        fprintf(stderr,"Input non valido\n");
        exit(EXIT_FAILURE);
    }

    shared_data s = {.N = n_vectors, .M = dim, .accepted = 0, .n_verifiers=VERIFIERS};
    if(pthread_mutex_init(&s.mtx, NULL) != 0){
        fprintf(stderr,"Errore durante la creazione del mutex");
        exit(EXIT_FAILURE);
    }

    queue *proposals = calloc(1, sizeof(queue));
    queue *discarded = calloc(1, sizeof(queue));
    if(!proposals){
        fprintf(stderr,"Errore durante la creazione della coda\n");
        exit(EXIT_FAILURE);
    }

    if(!discarded){
        fprintf(stderr,"Errore durante la creazione della coda\n");
        free(proposals);
        exit(EXIT_FAILURE);
    }


    if(initqueue(proposals, PROPOSALS_MAX) != 0){
        pthread_mutex_destroy(&s.mtx);
        exit(EXIT_FAILURE);
    }

    if(initqueue(discarded, DISCARDED_MAX) != 0){
        destroyqueue(proposals);
        pthread_mutex_destroy(&s.mtx);
        exit(EXIT_FAILURE);
    }
    generator_arg ga = {.s = &s, .q = proposals};

    
    pthread_create(&ga.tid, NULL, thread_generator, &ga);


    
    verificator_arg vas[VERIFIERS];


    for(int i = 0; i < VERIFIERS; i++){
        vas[i].id = i+1;
        vas[i].discarded = discarded;
        vas[i].proposals = proposals;
        vas[i].s = &s;
        pthread_create(&vas[i].tid, NULL, thread_verificator, &vas[i]);
    }

    repairer_arg ras[REPAREIRS];
    

    for(int i = 0; i < REPAREIRS; i++){
        ras[i].id = i+1;
        ras[i].discarded = discarded;
        ras[i].proposals = proposals;
        ras[i].s = &s;
        pthread_create(&ras[i].tid, NULL, thread_repairer, &ras[i]);
    }

    pthread_join(ga.tid, NULL);
    for(int i = 0; i < VERIFIERS; i++){
        pthread_join(vas[i].tid, NULL);
    }
    for(int i = 0; i < REPAREIRS; i++){
        pthread_join(ras[i].tid, NULL);
    }
    destroyqueue(proposals);
    destroyqueue(discarded);
    pthread_mutex_destroy(&s.mtx);
    free(proposals);
    free(discarded);
    fprintf(stdout,"[MAIN] terminato\n");
}
