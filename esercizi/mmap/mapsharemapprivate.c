#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>

#define MODE 0644

void write_private_mapped(int fd, ssize_t size){
    char *data;
    if((data = mmap(NULL,size,PROT_READ | PROT_WRITE, MAP_PRIVATE,fd,0))== MAP_FAILED){
        perror("mmap");
        return;
    }

    if(size >= 4){
        data[0] = 'C';
        data[1] = 'I';
        data[2] = 'A';
        data[3] = 'O';
    }
    
    msync(data,size,MS_SYNC);
    munmap(data,size);
}

void write_shared_mapped(int fd, ssize_t size){
    char *data;
    if((data = mmap(NULL,size,PROT_READ | PROT_WRITE, MAP_SHARED,fd,0)) == MAP_FAILED){
        perror("mmap");
        return;
    }


    if(size >= 4){
        data[0] = 'C';
        data[1] = 'I';
        data[2] = 'A';
        data[3] = 'O';
    }

    msync(data,size,MS_SYNC);
    munmap(data,size);
}


void read_file_mapped(int fd, ssize_t size){
    char *data;
    if((data = mmap(NULL,size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,0)) == MAP_FAILED){
        perror("mmap");
        return;
    }

    if(write(STDOUT_FILENO,data,size) != size){
        perror("write on stdout");
    }

    munmap(data,size);
}
int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr,"USAGE: %s <path>",argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd = open(argv[1],O_RDWR,MODE);
    if(fd == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }

    struct stat sf;
    if(fstat(fd,&sf) == -1){
        perror("fstat");
        exit(EXIT_FAILURE);
    }

    ssize_t length = sf.st_size;


    fprintf(stdout,"FILE BEFORE CHANGES:\n");
    read_file_mapped(fd,length);


    fprintf(stdout,"-------APPLY MODIFY WITH MAP_PRIVATE-------\n");
    sleep(1);
    write_private_mapped(fd,length);
    fprintf(stdout,"FILE AFTER CHANGES APPLIED BY MAP_PRIVATE:\n");
    read_file_mapped(fd,length);


    fprintf(stdout,"-------APPLY MODIFY WITH MAP_SHARED-------\n");
    sleep(1);
    write_shared_mapped(fd,length);
    fprintf(stdout,"FILE AFTER CHANGES APPLIED BY MAP_SHARED:\n");
    read_file_mapped(fd,length);


}