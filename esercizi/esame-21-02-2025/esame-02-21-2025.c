#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>

typedef struct{
    long long op1;
    long long op2;
    char operation;
    long long result;

    int round;

    bool done1, done2, dones;
    bool ex1, ex2, excal;

    bool error;

    pthread_cond_t extrapolate, calculate;
    pthread_mutex_t mtx;
} shared_data;


typedef struct{
    pthread_t tid;
    shared_data *s;
    char *path;
} thread_data;







void *thread_op1(void *arg){
    thread_data *td = (thread_data *) arg;
    shared_data *s = td->s;
    FILE *fp = fopen(td->path,"r");


    if(!fp){
        fprintf(stderr,"[OP1] errore durante l'apertura del file\n");
        exit(EXIT_FAILURE);
    }else{
        fprintf(stderr,"[OP1] leggo gli operandi dal file \'%s\'\n",td->path);
    }


    for(;;){
        pthread_mutex_lock(&s->mtx);
        while(s->done1){
            pthread_cond_wait(&s->extrapolate, &s->mtx);
        }
        char buffer[10];
        if(fgets(buffer,sizeof(buffer),fp) == NULL){
            s->ex1 = true;
            fprintf(stdout,"[OP1] termino\n");
            pthread_cond_broadcast(&s->calculate);
            pthread_mutex_unlock(&s->mtx);
            break;
        }
        s->op1 = (long long) strtol(buffer, NULL, 10);
        fprintf(stderr,"[OP1] primo operando n.%d: %lld",s->round, s->op1);
        
        s->done1 = true;
        pthread_cond_broadcast(&s->calculate);
        pthread_mutex_unlock(&s->mtx);
    }
    fclose(fp);
    return NULL;
}



void *thread_op2(void *arg){
    thread_data *td = (thread_data *)arg;
    shared_data *s = td->s;
    FILE *fp = fopen(td->path,"r");


    if(!fp){
        fprintf(stderr,"[OP2] errore durante l'apertura del file\n");
        exit(EXIT_FAILURE);
    }else{
        fprintf(stderr,"[OP2] leggo gli operandi dal file \'%s\'\n",td->path);
    }


    for(;;){
        pthread_mutex_lock(&s->mtx);
        while(s->done2){
            pthread_cond_wait(&s->extrapolate, &s->mtx);
        }
        char buffer[10];
        if(fgets(buffer,sizeof(buffer),fp) == NULL){
            s->ex2 = true;
            fprintf(stdout,"[OP2] termino\n");
            pthread_cond_broadcast(&s->calculate);
            pthread_mutex_unlock(&s->mtx);
            break;
        }
        s->op2 = (long long) strtol(buffer, NULL, 10);
        fprintf(stderr,"[OP2] secondo operando n.%d: %lld",s->round, s->op2);
        s->done2 = true;
        pthread_cond_broadcast(&s->calculate);
        pthread_mutex_unlock(&s->mtx);
    }
    fclose(fp);
    return NULL;
}





void *thread_ops(void *arg){
    thread_data *td = (thread_data *) arg;
    shared_data *s = td->s;
    FILE *fp = fopen(td->path, "r");
    if(!fp){
        fprintf(stderr, "[OPS] errore durante l'apertura del file\n");
        exit(EXIT_FAILURE);
    }else{
        fprintf(stdout, "[OPS] leggo le operazioni e il risultato atteso dal file \'%s\'\n", td->path);
    }

    for(;;){
        pthread_mutex_lock(&s->mtx);
        while(s->dones){
            pthread_cond_wait(&s->extrapolate, &s->mtx);
        }
        if(s->round >1){
            fprintf(stdout,"[OPS]sommatoria dei risultati parziali dopo %d operazione/i: %lld\n", s->round, s->result);
        }
        char buffer[10];
        if(fgets(buffer,10,fp) == NULL){
            fprintf(stderr,"[OPS] errore durante la lettura dal file\n");
            exit(EXIT_FAILURE);
        }
        if(s->excal){
            long long v = ( long long ) strtol(buffer,NULL,10);
            if(v == s->result){
                fprintf(stdout,"[OPS] risultato finale atteso: %lld (corretto)\n", v);
            }else{
                fprintf(stdout,"[OPS] risultato finale atteso: %lld (errato)\n", v);
            }
            pthread_mutex_unlock(&s->mtx);
            break;
        }else{
            char c = buffer[0];
            if(c != '+' && c != '-' && c!= 'X' && c!= '*' && c!= 'x'){
                fprintf(stderr,"[OPS] operazione non identificata\n");
                exit(EXIT_FAILURE);
            }
            s->operation = c;
            fprintf(stdout,"[OPS] operazione n.%d: %c\n", s->round, s->operation);
            s->dones = true;
            pthread_cond_broadcast(&s->calculate);
            pthread_mutex_unlock(&s->mtx);
        }
    }
    fclose(fp);
    return NULL;
}


void *thread_cal(void *arg){
    thread_data *td = (thread_data *)arg;
    shared_data *s = td->s;
    for(;;){
        pthread_mutex_lock(&s->mtx);
        if(s->ex1 && s->ex2){
            s->excal = true;
            fprintf(stdout,"[CALC] termino\n");
            pthread_cond_broadcast(&s->calculate);
            pthread_mutex_unlock(&s->mtx);
            break;
        }
        while(!s->done1 || !s->done2 || !s->dones){
            pthread_cond_wait(&s->calculate, &s->mtx);
        }
        long long result;
        switch(s->operation){
            case '+':
                result = s->op1+s->op2;
                fprintf(stdout,"[CALC] operazione minore n.%d : %lld + %lld = %lld\n",
                s->round, s->op1, s->op2, result);
                s->result += result;
                s->round++;
            break;
            case '-':
                result = s->op1-s->op2;
                fprintf(stdout,"[CALC] operazione minore n.%d : %lld - %lld = %lld\n",
                s->round, s->op1, s->op2, result);
                s->result += result;
                s->round++;
            break;
            case '*':
            case 'x':
            case 'X':
                result = s->op1*s->op2;
                fprintf(stdout,"[CALC] operazione minore n.%d : %lld * %lld = %lld\n",
                s->round, s->op1, s->op2, result);
                s->result += result;
                s->round++;
            break;
            default: 
            break;
        }
        s->done1 = false;
        s->done2 = false;
        s->dones = false;
        pthread_cond_broadcast(&s->extrapolate);
        pthread_mutex_unlock(&s->mtx);
    }

    
    return NULL;
}



int main(int argc, char **argv){
    if(argc != 4){
        fprintf(stderr,"Uso: %s <file_op1> <file_op2> <file_ops>\n", argv[0]);
        return EXIT_FAILURE;
    }

    
    shared_data s;
    s.op1 = s.op2 = s.result = 0;
    s.round = 0;
    s.done1 = s.done2 = s.dones = false;
    s.ex1 = s.ex2 = s.excal = false;
    s.error = false;
    pthread_mutex_init(&s.mtx, NULL);
    pthread_cond_init(&s.calculate, NULL);
    pthread_cond_init(&s.extrapolate, NULL);

    thread_data t1 = {.s = &s, .path = argv[1]};
    thread_data t2 = {.s = &s, .path = argv[2]};
    thread_data t3 = {.s = &s, .path = argv[3]};
    thread_data tcal = {.s = &s, .path = NULL};

    if(pthread_create(&t1.tid, NULL, thread_op1, &t1) != 0){
        perror("pthread_create OP1");
        return EXIT_FAILURE;
    }
    if(pthread_create(&t2.tid, NULL, thread_op2, &t2) != 0){
        perror("pthread_create OP2");
        return EXIT_FAILURE;
    }
    if(pthread_create(&t3.tid, NULL, thread_ops, &t3) != 0){
        perror("pthread_create OPS");
        return EXIT_FAILURE;
    }
    if(pthread_create(&tcal.tid, NULL, thread_cal, &tcal) != 0){
        perror("pthread_create CALC");
        return EXIT_FAILURE;
    }

    pthread_join(t1.tid, NULL);
    pthread_join(t2.tid, NULL);
    pthread_join(t3.tid, NULL);
    pthread_join(tcal.tid, NULL);

    pthread_mutex_destroy(&s.mtx);
    pthread_cond_destroy(&s.calculate);
    pthread_cond_destroy(&s.extrapolate);

    printf("[MAIN] Tutti i thread hanno terminato. Risultato finale: %lld\n", s.result);

    return EXIT_SUCCESS;
}
