/*
Programma C
* Dato un file specificato da riga di comando ne conta il numero di linee operando come segue:
* - Prima crea 8 sottoprocessi.
* - Ognuno di questi provvede ad esaminare una sezione del file input pari ad 1/8 della taglia totale del file.
* - Una volta terminati tutti i figli, il processo padre stamperà la somma dei valori resituiti da ciascun figlio
* Per restituire i 3 valori si suggerisce di utilizzare una pipe.
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <errno.h>

#define WRITE_END 1
#define READ_END 0

#define N_PROC 8

struct packet{
    int newLines;
    int childNum;
};

int childWorker(char *file, off_t offsetStart, off_t size, int numChild, int pipefd[2]);
int parentWorker(int (*)[2], int *);

int main(int argc, char * argv[]){

    if( argc < 2){
        fprintf(stderr, "Usage %s 'path/to/file \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int pipefd[N_PROC][2];
    int pid[N_PROC];

    struct stat sb;
    off_t sizePerChild[N_PROC];

    /* Get File Info */
    if( (stat(argv[1], &sb)) < 0 ){
        fprintf(stderr, "Error while executing stat : %s \n", strerror(errno));
        exit(1);
    }

    /* Calculate offset for each child  */
    sizePerChild[0] = sb.st_size/N_PROC;
    for(int i = 1; i < N_PROC-1; i++){
        sizePerChild[i]  = sizePerChild[0];
    }
    sizePerChild[N_PROC-1] = sb.st_size - sizePerChild[0]*(N_PROC-1);

    /* Create N_PROC pipes and child */
    for(int i = 0; i < N_PROC; i++){
        if( (pipe2(pipefd[i], O_NONBLOCK)) < 0 ){
           fprintf(stderr, "Error while creating pipe num %d : %s \n", i, strerror(errno)); 
           exit(1);
        }

        if( (pid[i] = fork()) < 0 ){ //Error Handling
            fprintf(stderr, "Error while creating child num %d : %s \n", i, strerror(errno)); 
        
        } else if(pid[i] == 0){ // i-th child

            /*Close Unused END of each pipe*/
            for(int j = 0; j <= i; j++){
                close(pipefd[j][READ_END]);
                if( j != i) close(pipefd[j][WRITE_END]);
            }

            /*Send to ChildWorker*/
            childWorker(argv[1], i*sizePerChild[0], sizePerChild[i], i, pipefd[i]);
            exit(1); //Should not go here, JIC.
        }

    }

    //Only Parent Here
    for(int i = 0; i < N_PROC; i++){
        close(pipefd[i][WRITE_END]);
    }

    parentWorker(pipefd, pid);

    exit(EXIT_SUCCESS);
}

/** ----- CHILD ----- **/

int childWorker(char *file, off_t offsetStart, off_t size, int numChild, int pipefd[2]){

    char buffer[4096] = {0};
    ssize_t bytesRead = 0;
    int fd;
    int counter = 0;
    off_t sizeRead = 0;

    struct packet send;

    //printf("Child Data: num %d, Start %ld, Size %ld \n", numChild, offsetStart, size);

    /*Open File*/
    if( (fd = open(file, O_RDONLY)) < 0 ){
        fprintf(stderr, "Error while opening file in child num %d : %s \n", numChild, strerror(errno)); 
        exit(1);
    }

    /*Setting offset*/
    if( (lseek(fd, offsetStart, SEEK_SET)) < 0 ){
        fprintf(stderr, "Error while lseek in child num %d : %s \n", numChild, strerror(errno)); 
        exit(1);
    }

    size_t bytes = (off_t) sizeof(buffer) < size ? sizeof(buffer) : (size_t) size;

    /*read num of \n for each section*/
    while ( (bytesRead = read(fd, buffer, bytes)) > 0 ){
        char * ptr = buffer;

        while(*ptr != '\0'){
            if(*(ptr++) == '\n'){ counter++; }
        }

        sizeRead += bytesRead;
        memset(buffer, 0, 4096);
    
        if(sizeRead >= size){
            bytesRead = 0;
            break;
        }
        
    }
    if( (bytesRead < 0) ){
        fprintf(stderr, "Error while reading in child num %d : %s \n", numChild, strerror(errno)); 
    
    } else {
        printf("Child %d: Done Reading \n", numChild);
    }

    close(fd);

    send.childNum = numChild;
    send.newLines = counter;

    if( (write(pipefd[WRITE_END], &send, sizeof(struct packet))) < 0){
       fprintf(stderr, "Error while sending to par from child num %d : %s \n", numChild, strerror(errno));  
       exit(1);
    }
    
    close(pipefd[WRITE_END]);

    exit(0);
}

/** ----- PARENT ----- **/

int parentWorker(int (*pipefd)[2], int * pid){
    int count = N_PROC;
    int i, pos = 0;
    ssize_t bytesRead;
    struct packet received[N_PROC];
    struct packet buffer;
    int sum = 0;

    while(count){

        for(i = 0; i < N_PROC; i++){
            if(pid[i] == 0) continue;
            
            else if( (bytesRead = read(pipefd[i][READ_END], &buffer, sizeof(struct packet))) < 0 ){
                if(errno != EWOULDBLOCK && errno != EAGAIN){
                    fprintf(stderr, "Error while reading from par in pipe child %d: %s \n", i, strerror(errno));  
                    exit(1);
                }
            } else if(bytesRead == 0){
                pid[i] = 0;
                count--;
            } else if(bytesRead > 0){
                received[pos].newLines = buffer.newLines;
                received[pos++].childNum = buffer.childNum;
            }
        }

        usleep(10000); //sleep for 10ms
    }

    //Wait End of Child
    for(int i = 0; i < N_PROC; i++){
        waitpid(pid[i], NULL, WUNTRACED);
    }

    printf("\nFrom Parent: num of \\n: \n");

    for(int i = 0; i < N_PROC; i++){
        printf("From Child %d = %d \n", received[i].childNum, received[i].newLines);
        sum += received[i].newLines;
    }

    printf("\nSUM = %d \n\n", sum);

    return 0;
}
