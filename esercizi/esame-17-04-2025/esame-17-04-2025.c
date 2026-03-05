#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/dirent.h>
#include <dirent.h>
#include <libgen.h>


#define BUFF_SZ 1024
#define PATH_SZ 256
#define FILE_NMSZ 128
#define STACK_SZ 10


typedef struct{
    char BUFFER[BUFF_SZ];
    char filename[PATH_SZ];
    size_t dim_file;
    off_t cursor;
    size_t len;
    bool eof;
    bool error;
}block;

typedef struct {
    block b[STACK_SZ];
    int count;

    pthread_cond_t not_full;
    pthread_cond_t not_empty;
    pthread_mutex_t mtx;
} stack;


typedef struct{
    int id;
    char src_path[PATH_SZ];
    pthread_t tid;
    stack *s;
} reader_arg;

typedef struct{
    stack *s;
    const char *dest_dir;
    pthread_t tid;
    int total_files;
} writer_arg;



typedef struct{
    char filename[FILE_NMSZ];
    bool created;
} created;


int stack_init(stack *s){
    s->count = 0;
    pthread_cond_init(&s->not_empty, NULL);
    pthread_cond_init(&s->not_full, NULL);
    pthread_mutex_init(&s->mtx, NULL);
    return 0;
}

void stack_destroy(stack *s){
    pthread_cond_destroy(&s->not_empty);
    pthread_cond_destroy(&s->not_full);
    pthread_mutex_destroy(&s->mtx);
}


void push(stack *s, const block *b){
    pthread_mutex_lock(&s->mtx);
    while(s->count == STACK_SZ){
        pthread_cond_wait(&s->not_full, &s->mtx);
    }
    s->b[s->count++] = *b;
    pthread_cond_signal(&s->not_empty);
    pthread_mutex_unlock(&s->mtx);
}

void pop(stack *s, block *b){
    pthread_mutex_lock(&s->mtx);
    while(s->count == 0){
        pthread_cond_wait(&s->not_empty, &s->mtx);
    }
    *b = s->b[--s->count];
    pthread_cond_signal(&s->not_full);
    pthread_mutex_unlock(&s->mtx);
}


void *thread_reader(void *arg){
    reader_arg *rarg = (reader_arg *) arg;
    int fd = open(rarg->src_path, O_RDONLY);
    
    if(fd == -1){
        fprintf(stderr,"[READER-%d] errore durante l'apertura del file, termino\n", rarg->id);
        block b;
        b.error = true;
        push(rarg->s,&b);
        pthread_exit(NULL);
    }
    struct stat st;
    if(fstat(fd, &st) == -1){
        fprintf(stderr,"[READER-%d] errore durante l'apertura del file, termino\n", rarg->id);
        block b;
        b.error = true;
        push(rarg->s,&b);
        pthread_exit(NULL);
    }

    char filename[FILE_NMSZ];
    strncpy(filename, basename(rarg->src_path), FILE_NMSZ);
    size_t len = 0;
    off_t oft = 0;
    char buffer[BUFF_SZ];
    fprintf(stdout,"[READER-%d] lettura del file \'%s\' di %lld byte\n", rarg->id, rarg->src_path, st.st_size);
    while((len = read(fd, buffer, BUFF_SZ)) > 0){
        oft = lseek(fd, 0, SEEK_CUR) - len;
        block b;
        memcpy(b.BUFFER, buffer, len);
        b.cursor = oft;
        b.dim_file = st.st_size;
        strcpy(b.filename, filename);
        b.len = len;
        fprintf(stdout,"[READER-%d] lettura del blocco di offset %lld di %zu byte\n", rarg->id, b.cursor, b.len);
        b.eof = (len < BUFF_SZ);
        push(rarg->s, &b);
    }
    fprintf(stdout,"[READER-%d] lettura del file %s completata\n", rarg->id, rarg->src_path);
    pthread_exit(NULL);
}

void *thread_writer(void *arg){
    writer_arg *warg = (writer_arg *) arg;


    struct dirent d;
    char path[PATH_SZ];
    DIR *current = opendir(".");
    struct dirent *dir;
    bool isin = false;


    while((dir = readdir(current))){
        if(strcmp(dir->d_name,warg->dest_dir) == 0){
            isin = true;
        }
    }


    if(!isin){
        if(mkdir(warg->dest_dir, 0755) == -1){
            perror("mkdir");
            exit(EXIT_FAILURE);
        }
    }

    int created = 0;
    int exited = 0;

    char **filecreated = calloc(warg->total_files,sizeof(char *));
    for(int i=0; i< warg->total_files; i++){
        filecreated[i] = calloc(FILE_NMSZ,sizeof(char));
    }

    stack *s = warg->s;
    while((exited < warg->total_files) && (s->count == 0)){
        block b;
        pop(s,&b);
        int fd;
        char path[PATH_SZ];
        sprintf(path,"%s/%s", warg->dest_dir, b.filename);
        bool exist = false;
        if(b.error){
            exited++;
            continue;
        }


        for(int i= 0; i < created; i++){
            if(strcmp(filecreated[i],b.filename)==0){
                exist = true;
                break;
            }
        }

        if(!exist){
            fprintf(stdout,"[WRITER] creazione del file \'%s\' di dimensione %zu byte\n", b.filename, b.dim_file);
            strcpy(filecreated[created++],b.filename);
            fd = open(path,O_CREAT | O_WRONLY, 0644);
        }else{
            fd = open(path, O_WRONLY);
        }
        if(fd == -1){
            fprintf(stderr,"[WRITER] errore durante la creazione del file \'%s\'\n", b.filename);
            exit(EXIT_FAILURE);
        }
        


        if(pwrite(fd,b.BUFFER, b.len, b.cursor) != b.len){
            fprintf(stderr,"[WRITER] errore durante la creazione del file \'%s\'\n", b.filename);
            exit(EXIT_FAILURE);
        }
        fprintf(stderr,"[WRITER] scrittura del blocco di offset %lld di %zu sul file \'%s\'\n", b.cursor, b.len, b.filename);
        close(fd);
        if(b.eof){
            exited++;
        }
    }

    for(int i = 0; i < warg->total_files; i++){
        free(filecreated[i]);
    }

    fprintf(stdout,"[WRITER] scritti tutti i file in \'%s\'\n", warg->dest_dir);

    free(filecreated);

    pthread_exit(NULL);
}


int main(int argc, char **argv){
    if(argc < 3){
        fprintf(stderr,"USO: %s <file-1> <file-2> ... <file-n> <destination-dir>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n_files = argc - 2;
    char *dest_dir = argv[argc - 1];

    fprintf(stdout,"[MAIN] duplicazione di %d file\n", n_files);
    stack s;
    stack_init(&s);

    pthread_t writer_tid;
    pthread_t reader_tid[n_files];

    reader_arg rargs[n_files];
    writer_arg warg;

    
    warg.s = &s;
    warg.dest_dir = dest_dir;
    warg.total_files = n_files;
    

    if(pthread_create(&writer_tid, NULL, thread_writer, &warg) != 0){
        perror("pthread_create writer");
        exit(EXIT_FAILURE);
    }

    
    for(int i = 0; i < n_files; i++){
        rargs[i].id = i;
        strncpy(rargs[i].src_path, argv[i+1], PATH_SZ);
        rargs[i].s = &s;

        if(pthread_create(&reader_tid[i], NULL, thread_reader, &rargs[i]) != 0){
            perror("pthread_create reader");
            exit(EXIT_FAILURE);
        }
    }

    for(int i = 0; i < n_files; i++){
        pthread_join(reader_tid[i], NULL);
    }
    

    pthread_join(writer_tid, NULL);

    stack_destroy(&s);
    fprintf(stdout,"[MAIN] duplicazione di %d file completata\n", n_files);
    return 0;
}
