#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>


#define LINE_SZ 512
#define OBJ_SZ 256




typedef struct{
    char name[OBJ_SZ];
    int min;
    int max;

    pthread_mutex_t mtx;
    pthread_cond_t cond_start, cond_report;
    
    int *offers;
    int *valid;
    int *rank;
    int *order;

    int num_bidders;
    int round_id;


    int reported;
    int received;

    int arrival_seq;

    int exit_flag;
    int auction_index;
} shared_data;


typedef struct {
    pthread_t tid;
    shared_data *S;
    int id;
} bidder_arg;



int extrapolate(char *str, char *name, int *min, int *max){
    int size = strlen(str);
    if(str[size-1] == '\n' || str[size-1] == '\r'){
        str[size-1] = '\0';
    }
    char *n = strtok(str,",");
    char *mi = strtok(NULL,",");
    char *ma = strtok(NULL,",");

    memcpy(name,n,OBJ_SZ-1);
    *min = atoi(mi);
    *max = atoi(ma);

    if(*min < 0 || (*max < *min)){
        return -1;
    }
    return 0;
}


int pick_winner(shared_data *S, int *best_value, int *n_valid){
    int max = -1;
    int winner = -1;
    int count_valid = 0;

    for(int i = 0; i < S->num_bidders; i++){
        if(!S->valid[i]){
            continue;
        }
        count_valid++;
        if(S->offers[i] > max){
            max = S->offers[i];
            winner = i;
        } else if( S->offers[i] == max && winner >= 0){
            if(S->rank[i] < S->rank[winner]){
                winner = i;
            }
        }
    }

    *best_value = (winner >= 0 ? max : -1);
    *n_valid = count_valid;
    return winner;
}

void bidder_load_value_locked(shared_data *s, int id, int offer){
    s->offers[id] = offer;
    s->valid[id] = (offer >= s->min && offer <= s->max)? 1 : 0;
    s->rank[id] = s->arrival_seq;
    s->order[s->arrival_seq] = id;


    s->arrival_seq++;
    s->received++;
    pthread_cond_signal(&s->cond_report);
}



void *bidder_function(void *arg){
    bidder_arg *b = (bidder_arg *)arg;
    shared_data *s = b->S;
    int idx = b->id;
    int last_round = -1;
    pthread_mutex_lock(&s->mtx);
    for(;;){
        while(!s->exit_flag && last_round == s->round_id){
            pthread_cond_wait(&s->cond_start,&s->mtx);
        }
        if(s->exit_flag){
            pthread_mutex_unlock(&s->mtx);
            break;
        }
        last_round = s->round_id;
        int my_auction_index = s->auction_index;
        int max = s->max;

        int offer = 1 + rand()%max;
        printf("[B%d] invio offerta di %d EUR per asta n.%d\n", b->id + 1, offer, my_auction_index);

        bidder_load_value_locked(s, b->id, offer);

    }
    pthread_exit(NULL);
}

int main(int argc, char **argv){
    if(argc != 3){
        fprintf(stderr,"USO: %s <auction-file> <num-bidders>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *filename = argv[1];
    int num_bidders = atoi(argv[2]);
    if(num_bidders <= 0){
        fprintf(stderr,"\'num-bidders\' deve essere maggiore di 0\n");
        exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(filename, "r");
    if(!fp){
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    
    shared_data S;
    memset(&S,0,sizeof(S));
    

    if(pthread_mutex_init(&S.mtx, NULL) != 0){
        fprintf(stderr,"MUTEX NON INIZIALIZZATO\n");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    if((pthread_cond_init(&S.cond_start, NULL) != 0) || (pthread_cond_init(&S.cond_report, NULL) != 0)){
        fprintf(stderr,"VARIABILI CONDIZIONE NON INIZIALIZZATE\n");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    S.offers = calloc(sizeof(int), num_bidders);
    S.valid = calloc(sizeof(int), num_bidders);
    S.order = calloc(sizeof(int), num_bidders);
    S.rank = calloc(sizeof(int), num_bidders);

    if(!S.offers || !S.valid || !S.order || !S.rank){
        perror("calloc");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    S.num_bidders = num_bidders;

    bidder_arg *bidders = calloc(sizeof(bidder_arg), num_bidders);
    for(int i = 0; i < num_bidders; i++){
        bidders[i].id = i;
        bidders[i].S = &S;
        if(pthread_create(&bidders->tid, NULL, bidder_function, &bidders[i]) != 0){
            fprintf(stderr,"ERRORE DURANTE LA CREAZIONE DEI THREAD\n");
            exit(EXIT_FAILURE);
        }
    }

    int total_auction = 0;
    int voided = 0;
    int assigned = 0;
    long long total_revenue = 0;

    char line[LINE_SZ];

    int auction_id = 1;
    
    while(fgets(line,sizeof(line),fp)){
        char name[OBJ_SZ];
        int min, max;
        if(extrapolate(line, name, &min, &max) ==-1){
            fprintf(stderr,"STRINGA NON VALIDA PER L'ESTRAZIONE DELLE INFORMAZIONI\n");
            continue;
        }
        pthread_mutex_lock(&S.mtx);

        fprintf(stdout,"[J] lancio asta n.%d per %s "
            "con offerta minima di %d EUR e massima di %d EUR\n",
        auction_id, name, min, max);
        
        S.auction_index = auction_id;
        strncpy(S.name,name, OBJ_SZ-1);
        S.min = min;
        S.max = max;
        S.received = 0;
        S.reported = 0;
        S.arrival_seq = 0;

        for(int i = 0; i < num_bidders; i++){
            S.offers[i] = 0;
            S.valid[i] = 0;
            S.order[i] = -1;
            S.rank[i] = 0;
        }

        S.round_id++;
        pthread_cond_broadcast(&S.cond_start);

        while(S.received < S.num_bidders){
            while(S.reported < S.received){
                int idx = S.order[S.reported];
                if(idx >= 0 && idx < S.num_bidders){
                    fprintf(stdout, "[J] ricevuta offerta da B%d\n", idx+1);
                }
                S.reported++;
            }
            if(S.received < S.num_bidders){
                pthread_cond_wait(&S.cond_report,&S.mtx);
            }
        }


        while(S.reported < S.received){
            int idx = S.order[S.reported];
            if(idx >= 0 && idx < S.num_bidders){
                fprintf(stdout,"[J] ricevuta offerta da B%d\n", idx+1);
            }
            S.reported++;
        }

        int best_value = -1, n_valid = 0;
        int winner = pick_winner(&S, &best_value, &n_valid);
        if(winner >= 0){
            fprintf(stdout,"[J] l'asta n.%d per %s si è conclusa per %d offerte valide"
                " su %d: il vincitore è B%d che si aggiudica l'oggetto per %d EUR.\n",
            auction_id, name, n_valid, S.num_bidders, winner+1, best_value);
            assigned++;
            total_revenue+=best_value;
        }else{
            fprintf(stdout, "[J] l'asta n.%d per %s si è conclusa senza alcuna offerta valida"
                " pertanto l'oggetto non risulta assegnato.\n",
            auction_id, name);
            voided++;
        }
        pthread_mutex_unlock(&S.mtx);
        total_auction++;
        auction_id++;

    }
    fclose(fp);

    pthread_mutex_lock(&S.mtx);
    S.exit_flag = 1;
    pthread_cond_broadcast(&S.cond_start);
    pthread_mutex_unlock(&S.mtx);

    for(int i = 0; i < num_bidders; i++){
        pthread_join(bidders[i].tid,NULL);
    }

    fprintf(stdout,"[J] sono state svolte %d aste di cui assegnate %d e %d andate a vuoto;"
        "il totale raccolto è di %lld EUR.\n", total_auction, assigned, voided, total_revenue);
    
    free(bidders);
    free(S.offers);
    free(S.order);
    free(S.valid);
    free(S.rank);

    pthread_cond_destroy(&S.cond_report);
    pthread_cond_destroy(&S.cond_report);
    pthread_mutex_destroy(&S.mtx);

    return EXIT_SUCCESS;

}