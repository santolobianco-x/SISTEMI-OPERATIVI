#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>


#define PATH_MAX 512
#define BUFFER_SIZE 256


typedef struct{
    long long operando_1;
    long long operando_2;
    long long risultato;
    char operazione;


    int richiedente;



    bool busy;
    bool calculated;



    pthread_cond_t operate;
    pthread_cond_t read;
    pthread_cond_t writable;
    pthread_mutex_t mtx;
    pthread_mutex_t mtx_calculator;



    int successfull;
    int n_readers;
} shared_data;


typedef struct{
    shared_data *s;
    pthread_t tid;
} operator_arg;


typedef struct{
    shared_data *s;
    pthread_t tid;
    int id;
    char filename[PATH_MAX];
    long long risultato;
} calculator_arg;


void *add(void *arg){
    operator_arg *opa = (operator_arg *)arg;
    shared_data *s = opa->s;


    for(;;){
        pthread_mutex_lock(&s->mtx);
        while((s->operazione != '+' ||s->calculated) && s->n_readers > 0){
            pthread_cond_wait(&s->operate, &s->mtx);
        }

        if(s->n_readers <= 0){
            pthread_mutex_unlock(&s->mtx);
            break;
        }


        s->risultato = s->operando_1+s->operando_2;
        s->calculated = true;
        fprintf(stdout,"[ADD] calcolo effettuato: %lld + %lld = %lld\n", s->operando_1, s->operando_2, s->risultato);
        pthread_cond_broadcast(&s->read);
        pthread_mutex_unlock(&s->mtx);
    }
    fprintf(stdout,"[ADD] terminazione");
    pthread_exit(NULL);
}



void *sub(void *arg){
    operator_arg *opa = (operator_arg *)arg;
    shared_data *s = opa->s;


    for(;;){
        pthread_mutex_lock(&s->mtx);
        while((s->operazione != '-' ||s->calculated) && s->n_readers > 0){
            pthread_cond_wait(&s->operate, &s->mtx);
        }

        if(s->n_readers <= 0){
            pthread_mutex_unlock(&s->mtx);
            break;
        }


        s->risultato = s->operando_1 - s->operando_2;
        s->calculated = true;
        fprintf(stdout,"[SUB] calcolo effettuato: %lld - %lld = %lld\n", s->operando_1, s->operando_2, s->risultato);
        pthread_cond_broadcast(&s->read);
        pthread_mutex_unlock(&s->mtx);
    }
    fprintf(stdout,"[SUB] terminazione");
    pthread_exit(NULL);
}




void *mul(void *arg){
    operator_arg *opa = (operator_arg *)arg;
    shared_data *s = opa->s;

    for(;;){
        pthread_mutex_lock(&s->mtx);
        while(((s->operazione != 'x' && s->operazione != 'X' && s->operazione != '*')||s->calculated) && s->n_readers > 0){
            pthread_cond_wait(&s->operate, &s->mtx);
        }

        if(s->n_readers <= 0){
            pthread_mutex_unlock(&s->mtx);
            break;
        }


        s->risultato = s->operando_1 * s->operando_2;
        s->calculated = true;
        fprintf(stdout,"[MUL] calcolo effettuato: %lld * %lld = %lld\n", s->operando_1, s->operando_2, s->risultato);
        pthread_cond_broadcast(&s->read);
        pthread_mutex_unlock(&s->mtx);
    }
    fprintf(stdout,"[MUL] terminazione");
    pthread_exit(NULL);
}



void *thread_reader(void *arg){
    calculator_arg *calca = (calculator_arg *) arg;
    FILE *fp = fopen(calca->filename, "r");

    shared_data *s = calca->s;
    if(!fp){
        fprintf(stderr,"Errore durante l'apertura del file\n");
        pthread_mutex_lock(&calca->s->mtx);
        calca->s->n_readers--;
        pthread_mutex_unlock(&calca->s->mtx);
        pthread_exit(NULL);
    }

    fprintf(stdout,"[CALC-%d] file da verificare: \'%s\'\n", calca->id, calca->filename);

    char buffer[BUFFER_SIZE];
    if(fgets(buffer, BUFFER_SIZE, fp) == NULL){
        fprintf(stderr,"Errore durante la lettura del file\n");
        pthread_mutex_lock(&calca->s->mtx);
        calca->s->n_readers--;
        pthread_mutex_unlock(&calca->s->mtx);
        pthread_exit(NULL);
    }
    
    
    calca->risultato = strtol(buffer, NULL, 10);
    bool success = false;

    while(fgets(buffer, BUFFER_SIZE, fp) != NULL){
        pthread_mutex_lock(&s->mtx_calculator);
        while(s->busy){
            pthread_cond_wait(&s->writable, &s->mtx_calculator);
        }

        pthread_mutex_lock(&s->mtx);

        
        s->busy = true;
        s->richiedente = calca->id;
        s->calculated = false;
        s->operando_1 = calca->risultato;

        size_t len = strlen(buffer);
        for(int i = 0; i < len; i++){
            if(buffer[i] == '\n' || buffer[i] == '\r'){
                buffer[i] = '\0';
            }
        }


        char first = buffer[0];
        if(!(first == '+' || first == 'x' || first == 'X' || first == '*' || (first == '-' && buffer[1] == ' '))){
            long long result = strtol(buffer, NULL, 10);
            if(result == s->operando_1){
                s->successfull++;
                success = true;
            }
            s->busy = false;
            pthread_mutex_unlock(&s->mtx);
            pthread_cond_broadcast(&s->writable);
            pthread_mutex_unlock(&s->mtx_calculator);
            break;
        }



        sscanf(buffer, "%c %lld", &s->operazione, &s->operando_2);

        fprintf(stdout,"[CALC-%d] prossima operazione: \'%s\'\n", calca->id, buffer);

        pthread_cond_broadcast(&s->operate);
        while(s->richiedente == calca->id && !s->calculated){
            pthread_cond_wait(&s->read, &s->mtx);
        }
        

        calca->risultato = s->risultato;
        s->busy = false;
        pthread_mutex_unlock(&s->mtx);
        pthread_cond_broadcast(&s->writable);
        fprintf(stdout,"[CALC-%d] risultato ricevuto: %lld\n", calca->id, s->risultato);
        pthread_mutex_unlock(&s->mtx_calculator);
    }

    if(success){
        fprintf(stdout,"[CALC-%d] computazione terminata in modo corretto: %lld\n", calca->id, calca->risultato);
    } else{
        fprintf(stdout,"[CALC-%d] computazione terminata in modo erroneo: %lld\n", calca->id, calca->risultato);
    }

    pthread_mutex_lock(&s->mtx);
    s->n_readers--;
    if(s->n_readers <= 0){
        pthread_cond_broadcast(&s->operate);
    }
    pthread_mutex_unlock(&s->mtx);
    pthread_exit(NULL);
}



int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Uso: %s <file1> <file2> ...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n_readers = argc - 1;

    shared_data s;

    // ================== INIT SHARED DATA ==================
    s.operando_1 = 0;
    s.operando_2 = 0;
    s.risultato = 0;
    s.operazione = '\0';
    s.richiedente = -1;

    s.busy = false;
    s.calculated = false;

    s.successfull = 0;
    s.n_readers = n_readers;

    pthread_mutex_init(&s.mtx, NULL);
    pthread_mutex_init(&s.mtx_calculator, NULL);

    pthread_cond_init(&s.operate, NULL);
    pthread_cond_init(&s.read, NULL);
    pthread_cond_init(&s.writable, NULL);

    operator_arg op_add, op_sub, op_mul;

    op_add.s = &s;
    op_sub.s = &s;
    op_mul.s = &s;

    pthread_create(&op_add.tid, NULL, add, &op_add);
    pthread_create(&op_sub.tid, NULL, sub, &op_sub);
    pthread_create(&op_mul.tid, NULL, mul, &op_mul);


    calculator_arg *calcs = malloc(n_readers * sizeof(calculator_arg));

    for (int i = 0; i < n_readers; i++) {

        calcs[i].s = &s;
        calcs[i].id = i;
        calcs[i].risultato = 0;

        strncpy(calcs[i].filename, argv[i + 1], PATH_MAX);

        pthread_create(&calcs[i].tid, NULL, thread_reader, &calcs[i]);
    }

    
    for (int i = 0; i < n_readers; i++) {
        pthread_join(calcs[i].tid, NULL);
    }

    
    pthread_mutex_lock(&s.mtx);
    pthread_cond_broadcast(&s.operate);
    pthread_mutex_unlock(&s.mtx);

    
    pthread_join(op_add.tid, NULL);
    pthread_join(op_sub.tid, NULL);
    pthread_join(op_mul.tid, NULL);

    
    printf("\n[MAIN] verifiche completate con successo: %d/%d\n", s.successfull, n_readers);

    
    pthread_mutex_destroy(&s.mtx);
    pthread_mutex_destroy(&s.mtx_calculator);

    pthread_cond_destroy(&s.operate);
    pthread_cond_destroy(&s.read);
    pthread_cond_destroy(&s.writable);

    free(calcs);

    return 0;
}
