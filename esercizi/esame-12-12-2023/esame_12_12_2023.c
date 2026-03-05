//COMPLETATO CON QUALCHE BUG
//->    IN QUESTO CASO CON MOLTI THREAD SI È PREFERITO UTILIZZARE DEI BOOLEANI
//      CHE SEGNASSERO I VARI CHECKPOINT RAGGIUNTI DAL PROGRAMMA.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>


#define NAME_MAX 256
#define GIFT_MAX 256
#define PATH_MAX 256
#define BUFFER_SIZE 512



typedef struct{
    char *name;
    bool good;
} good_bad_status;


typedef struct{
    char *gift;
    int price;
} price_arg;


typedef struct{
    char name[NAME_MAX];
    char gift[GIFT_MAX];
    bool isgood;
    int price;


    bool ready_to_read;
    bool go_to_babbonatale;

    bool be_judged;
    bool searched;
    bool notfound;


    bool ready_to_build;
    bool ready_to_record;


    int n_readers;


    



    pthread_mutex_t mtx;



    pthread_cond_t empty;
    pthread_cond_t received;
    pthread_cond_t investigate;
    pthread_cond_t produce;
    pthread_cond_t record;
} shared_data;


typedef struct{
    char *filename;
    shared_data *s;
    int id;
    pthread_t tid;
} es_arg;


typedef struct{
    shared_data *s;
    pthread_t tid;
} bn_arg;


typedef struct{
    char *filename;
    shared_data *s;
    good_bad_status *gbs;
    int num_children;
    pthread_t tid;
} ei_arg;


typedef struct {
    char *filename;
    price_arg *prices;
    int num_objects;
    shared_data *s;
    pthread_t tid;
} ep_arg;


typedef struct {
    shared_data *s;
    pthread_t tid;
    int received;
    int good;    
    int bad;
    int totalAmount;
} ec_arg;



void *thread_smistatore(void *arg){
    es_arg *es = (es_arg *)arg;
    shared_data *s = es->s;

    
    FILE *fp = fopen(es->filename, "r");
    if(!fp){
        fprintf(stderr,"[ES%d] Impossibile aprire il file \'%s\'\n", es->id, es->filename);
        pthread_mutex_lock(&s->mtx);
        s->n_readers--;
        if(s->n_readers == 0){
            pthread_cond_signal(&s->received);
            pthread_cond_signal(&s->investigate);
            pthread_cond_signal(&s->produce);
            pthread_cond_signal(&s->record);
        }
        pthread_mutex_unlock(&s->mtx);
        fprintf(stderr,"[ES%d] terminazione\n", es->id);
        pthread_exit(NULL);
    }
    
    fprintf(stdout,"[ES%d] leggo le letterine dal file \'%s\'\n", es->id, es->filename);
    char buffer[BUFFER_SIZE];

    while(fgets(buffer, BUFFER_SIZE, fp) != NULL){
        char *nm = strtok(buffer, ";\n");
        char *gft = strtok(NULL, ";\n");
        if(!nm || !gft){
            continue;
        }
        
        pthread_mutex_lock(&s->mtx);
        while(!s->ready_to_read){
            pthread_cond_wait(&s->empty,&s->mtx);
        }
        s->ready_to_read = false;

        
        strcpy(s->name, nm);
        strcpy(s->gift, gft);
        s->go_to_babbonatale = true;
        pthread_cond_signal(&s->received);
        pthread_mutex_unlock(&s->mtx);
    }

    
    fprintf(stdout,"[ES%d] non ho più letterine da consegnare\n", es->id);
    pthread_mutex_lock(&s->mtx);
    s->n_readers--;
    if(s->n_readers == 0){
        pthread_cond_signal(&s->received);
        pthread_cond_signal(&s->investigate);
        pthread_cond_signal(&s->produce);
        pthread_cond_signal(&s->record);
    }
    pthread_mutex_unlock(&s->mtx);
    pthread_exit(NULL);
}



void *thread_babbonatale(void *arg){
    bn_arg* bn = (bn_arg *)arg;
    shared_data *s = bn->s;
    
    for(;;){
        pthread_mutex_lock(&s->mtx);
        while(!s->go_to_babbonatale && s->n_readers > 0){
            pthread_cond_wait(&s->received, &s->mtx);
        }

        if(s->n_readers <= 0){
            pthread_mutex_unlock(&s->mtx);
            break;
        }
        fprintf(stdout,"[BN] come si è comportato il bambino \'%s\'?\n", s->name);


        s->be_judged = true;
        pthread_cond_signal(&s->investigate);
        

        while(!s->searched){
            pthread_cond_wait(&s->received, &s->mtx);
        }

        s->searched = false;

        if(s->notfound){
            fprintf(stdout,"[BN] il bambino \'%s\' non è presente nella lista\n", s->name);
            s->ready_to_read = true;
            s->searched = false;
            s->notfound = false;
            s->isgood = false;
            strcpy(s->name, " ");
            strcpy(s->gift, " ");
            pthread_cond_signal(&s->empty);
            pthread_mutex_unlock(&s->mtx);
            usleep(5000);
            continue;
        }

        


        if(s->isgood){
            fprintf(stdout,"[BN] il bambino \'%s\' riceverà il suo regalo \'%s\'\n", s->name, s->gift);
            s->ready_to_build = true;
            pthread_cond_signal(&s->produce);
        }else{
            fprintf(stdout,"[BN] il bambino \'%s\' non riceverà alcun regalo quest'anno!\n",s->name);
            s->ready_to_record = true;
            pthread_cond_signal(&s->record);
        }
        s->go_to_babbonatale = false; 
        pthread_mutex_unlock(&s->mtx);
    }
    
    
    fprintf(stdout,"[BN] non ci sono più bambini da esaminare\n");
    pthread_exit(NULL);
}



void *thread_indagatore(void *arg){
    ei_arg *ei = (ei_arg *)arg;
    shared_data *s = ei->s;
    ei->num_children = 0;
    FILE *fp = fopen(ei->filename, "r");
    

    if(!fp){
        fprintf(stderr,"[EI] errore durante l'apertura del file\n");
        pthread_mutex_lock(&s->mtx);
        //s->error = true;
        pthread_mutex_unlock(&s->mtx);
        pthread_exit(NULL);
    }
    
    char buffer[BUFFER_SIZE];

    bool allocationerror = false;
    while(fgets(buffer, BUFFER_SIZE, fp) != NULL){
        
        

        char *name = strtok(buffer,";\n");
        char *status = strtok(NULL,";\n");
        if(!name || !status){
            continue;
        }



        ei->gbs = realloc(ei->gbs,(ei->num_children+1)*sizeof(good_bad_status));
        if(ei->gbs == NULL){
            allocationerror = true;
            break;
        }

        
        ei->gbs[ei->num_children].name = strdup(name);
        bool isgood;


        if(strcmp(status,"buono") == 0){
            ei->gbs[ei->num_children].good = true;
        }else{
            ei->gbs[ei->num_children].good = false;
        }
        ei->num_children++;
        
    }
    

    if(allocationerror){
        fprintf(stderr,"[EI] errore durante l'allocazione del file\n");
        pthread_mutex_lock(&s->mtx);
        //s->error = true;
        pthread_mutex_unlock(&s->mtx);
        pthread_exit(NULL);
    }


    for(;;){
        
        pthread_mutex_lock(&s->mtx);
        while(!s->be_judged && s->n_readers > 0){
            pthread_cond_wait(&s->investigate, &s->mtx);
        }
        if(s->n_readers <= 0){
            pthread_mutex_unlock(&s->mtx);
            break;
        }
        
        s->notfound = true;
        for(int i = 0; i < ei->num_children; i++){
            if(strcmp(ei->gbs[i].name, s->name) == 0){
                s->notfound = false;
                s->isgood = ei->gbs[i].good;
                if(s->isgood){
                    fprintf(stdout,"[EI] il bambino '%s' è stato buono quest'anno\n", s->name);
                }else{
                    fprintf(stdout,"[EI] il bambino '%s' è stato cattivo quest'anno\n", s->name);
                }
                break;
            }
        }
        
        s->searched = true;
        s->be_judged = false;
        pthread_cond_signal(&s->received);
        pthread_mutex_unlock(&s->mtx);
    }
    
    fprintf(stdout,"[EI] non ci sono più bambini da esaminare\n");
    for(int i = 0; i < ei->num_children; i++){free(ei->gbs[i].name);}
    free(ei->gbs);
    pthread_exit(NULL);
}





void *thread_produttore(void *arg){
    ep_arg *ep = (ep_arg *)arg;
    shared_data *s = ep->s;
    ep->num_objects = 0;

    
    FILE *fp = fopen(ep->filename, "r");
    if(!fp){
        fprintf(stderr,"[EP] errore durante l'apertura del file\n");
        pthread_mutex_lock(&s->mtx);
        //s->error = true;
        pthread_mutex_unlock(&s->mtx);
        pthread_exit(NULL);
    }
    
    char buffer[BUFFER_SIZE];

    bool allocationerror = false;
    while(fgets(buffer, BUFFER_SIZE, fp) != NULL){
    
        char *gift = strtok(buffer,";\n");
        char *price = strtok(NULL,";\n");
        
        if(!gift || !price){
            continue;
        }


        ep->prices = realloc(ep->prices,(ep->num_objects+1)*sizeof(price_arg));
        if(ep->prices == NULL){
            allocationerror = true;
            break;
        }

        ep->prices[ep->num_objects].gift = strdup(gift);
        ep->prices[ep->num_objects].price = atoi(price);
        ep->num_objects++;
    }

    if(allocationerror){
        fprintf(stderr,"[EP] errore durante l'allocazione del file\n");
        pthread_mutex_lock(&s->mtx);
        //s->error = true;
        pthread_mutex_unlock(&s->mtx);
        pthread_exit(NULL);
    }
    


    for(;;){
        pthread_mutex_lock(&s->mtx);
        bool pricefound = false;
        while((!s->ready_to_build) && (s->n_readers > 0 )){
            pthread_cond_wait(&s->produce, &s->mtx);
        }
        
        if(s->n_readers <= 0){
            pthread_mutex_unlock(&s->mtx);
            break;
        }
        
        for(int i = 0; i < ep->num_objects; i++){
            if(strcmp(ep->prices[i].gift, s->gift) == 0){
                pricefound = true;
                s->price = ep->prices[i].price;
                fprintf(stdout,"[EP] creo il regalo '%s' per il bambino '%s' al costo di %d €\n", s->gift, s->name, s->price);
                break;
            }
        }

        if(!pricefound){
            fprintf(stdout,"[EP] regalo non in lista\n");
            s->price = 0;
        }
        
        s->ready_to_build = false;
        s->ready_to_record = true;
        pthread_cond_signal(&s->record);
        pthread_mutex_unlock(&s->mtx);
    }

    
    fprintf(stdout,"[EP] non ci sono più bambini da esaminare\n");
    for(int i = 0; i < ep->num_objects; i++){free(ep->prices[i].gift);}
    free(ep->prices);
    pthread_exit(NULL);
}


void *thread_contabile(void *arg){
    ec_arg *ec = (ec_arg *) arg;
    shared_data *s = ec->s;
    ec->bad = 0;
    ec->good = 0;
    ec->received = 0;
    ec->totalAmount = 0;

    for(;;){
        
        pthread_mutex_lock(&s->mtx);
        while((!s->ready_to_record && s->n_readers > 0)){
            pthread_cond_wait(&s->record, &s->mtx);
        }
        if(s->n_readers <= 0){
            pthread_mutex_unlock(&s->mtx);
            break;
        }

        
        if(!s->isgood){
            ec->bad++;
            fprintf(stdout,"[EC] aggiornate le statistiche dei bambini cattivi (%d)\n", ec->bad);   
        }else{
            ec->good++;
            
            ec->totalAmount += s->price;
            fprintf(stdout,"[EC] aggiornate le statistiche dei bambini buoni (%d) e dei costi totali (%d €)\n",
            ec->good, ec->totalAmount);
        }
        ec->received++;
        s->ready_to_read = true;
        s->isgood = false;

        s->go_to_babbonatale = false;
        s->be_judged = false;
        s->searched = false;
        s->notfound = false;
        s->ready_to_build = false;
        s->ready_to_record = false;

        s->price = 0;
        strcpy(s->name, " ");
        strcpy(s->gift, " ");
        
        pthread_cond_broadcast(&s->empty);
        pthread_mutex_unlock(&s->mtx);
        
    }
    
    fprintf(stdout,"[EC] quest'anno abbiamo ricevuto %d richieste"
        " da %d bambini buoni e da %d cattivi con un costo totale "
        "di produzione di %d €\n",ec->received, ec->good, ec->bad, ec->totalAmount);

    pthread_exit(NULL);
}







int main(int argc, char **argv){
    if(argc < 4){
        fprintf(stderr,"USO: %s <presents-file> <goods-bads-file> <letters-file-1> [... letters-file-n]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int n_readers = argc -3;
    shared_data s= {
    .ready_to_read=true, .isgood = false, .notfound = false, .be_judged = false, .searched = false,
    .ready_to_build = false, .ready_to_record = false,
    .n_readers = n_readers};
    strcpy(s.name, " ");
    strcpy(s.gift, " ");


    if(pthread_mutex_init(&s.mtx, NULL) != 0){
        fprintf(stderr,"Errore durante la creazione del mutex\n");
        exit(EXIT_FAILURE);
    }

    int e1 = pthread_cond_init(&s.empty, NULL);
    int e2 = pthread_cond_init(&s.produce, NULL);
    int e3 = pthread_cond_init(&s.received, NULL);
    int e4 = pthread_cond_init(&s.record, NULL);
    int e5 = pthread_cond_init(&s.investigate, NULL);
    if(e1 != 0 || e2 != 0 || e3 != 0 || e4 != 0 || e5 != 0){
        fprintf(stderr,"Errore durante la creazione delle variabili condizione\n");
        exit(EXIT_FAILURE);
    }


    ep_arg ep = {.filename = argv[1], .s = &s, .prices = NULL, .num_objects = 0};
    ec_arg ec = {.bad = 0, .good = 0, .received = 0, .s = &s, .totalAmount = 0};
    bn_arg bn = {.s = &s};
    ei_arg ei = {.filename = argv[2], .gbs = NULL, .num_children = 0, .s = &s};

    es_arg ess[n_readers];
    for(int i = 0; i < n_readers; i++){
        ess[i].filename = argv[i+3];
        ess[i].s = &s;
        ess[i].id = i+1;
        pthread_create(&ess[i].tid, NULL, thread_smistatore, &ess[i]);
    }

    pthread_create(&bn.tid, NULL, thread_babbonatale, &bn);
    pthread_create(&ei.tid, NULL, thread_indagatore, &ei);
    pthread_create(&ep.tid, NULL, thread_produttore, &ep);
    pthread_create(&ec.tid, NULL, thread_contabile, &ec);


    for(int i = 0; i < n_readers; i++){
        pthread_join(ess[i].tid, NULL);
    }

    pthread_join(bn.tid, NULL);
    pthread_join(ei.tid, NULL);
    pthread_join(ep.tid, NULL);
    pthread_join(ec.tid, NULL);

}