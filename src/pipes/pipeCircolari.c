/*
* Scrivere un programma C:
* Che crea 4 processi denominati A,B,C,D.
* A è il main e crea B, che a sua volta crea C che provvede a creare D.
* I 4 processi devono comunicare in maniera circolare attraverso l'uso delle pipe() Unix
* B aggiunge il proprio pid e invia la stringa a C che aggiunge il proprio pid e invia il messaggio a D
* che aggiunge il proprio pid e invia il messaggio ad A. 
* A invia a B una stringa di 8 cifre numeriche.
* Al termine di tutti i processi A invia un messaggio di terminazione (-1) e aspetta la terminazione di B./
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <errno.h>

#include <time.h>

#define WRITE_END 1
#define READ_END 0

#define N_PROC 4

char ps[] = {'A', 'B', 'C', 'D'};

pid_t pid[N_PROC];
int pipefd[N_PROC][2];

int childWorker(int childNum);
int parentWorker();

int counter = 1;
int main(){

    pid[0] = getpid();

    if( (pipe(pipefd[0])) < 0){
        fprintf(stderr, "Error Opening Pipe %d : %s \n", 0, strerror(errno));
        exit(1);
    }

    if( (pipe(pipefd[N_PROC-1])) < 0){
        fprintf(stderr, "Error Opening Pipe %d : %s \n", N_PROC -1, strerror(errno));
        exit(1);
    }

    if( (pid[counter] = fork()) < 0 ){ //Error
        fprintf(stderr, "Error Creating Child %d : %s \n", counter, strerror(errno));
        exit(1);

    } else if(pid[counter] > 0){
        parentWorker();

    } else {
        childWorker(counter++);
    }

    exit(EXIT_SUCCESS);
}

/** ----- CHILD ----- **/

int childWorker(int childNum){
    ssize_t bytesRead;
    char buffer[256] = {0};
    printf("I'm child %d \n", childNum);

    /*Create a Pipe and Fork from Child except if I'm the LAST child*/
    if(childNum < N_PROC-1){ //If it's not the last child
        
        if( (pipe(pipefd[childNum])) < 0){
            fprintf(stderr, "Error Opening Pipe in Ch %d : %s \n", childNum, strerror(errno));
            exit(1);
        } else {
            //printf("Opened Pipe %d\n", childNum);
        }

        if( (pid[counter] = fork()) < 0 ){
           fprintf(stderr, "Child: Error Creating Child %d : %s \n", childNum+1, strerror(errno));  
           exit(1);

        } else if( pid[counter] == 0){ //launch worker in new child
            childWorker(counter++);
            exit(1);
        }

    }

    /*Close unused pipes*/
    for(int i = 0; i <= N_PROC; i++){
        if(i != childNum) {close(pipefd[i][WRITE_END]);}
        if(i != childNum - 1) {close(pipefd[i][READ_END]);}
    }

    /*Read from previous pipe*/
    if( (bytesRead = read(pipefd[childNum - 1][READ_END], buffer, 256)) < 0 ){ //Error Handling
       fprintf(stderr, "Error reading from Child %d : %s \n", childNum-1, strerror(errno));  
       exit(1); 

    } else if(bytesRead > 0){
        char pidStr[15];
        sprintf(pidStr, "%c:%d-", ps[childNum], getpid());
        strcat(buffer, pidStr);

        //printf("Child %d: %s, %s\n", childNum, buffer, pidStr);
        
        /* Send to next pipe*/
        if( (write(pipefd[childNum][WRITE_END], buffer, strlen(buffer)+1)) < 0 ){
            fprintf(stderr, "Error writing from parent : %s \n", strerror(errno));  
            exit(1);
        }

    } else { //bytesRead == 0 DONE READING
        close(pipefd[childNum -1][READ_END]);
        close(pipefd[childNum][WRITE_END]);
    }

    exit(0);
}

/** ----- PARENT ----- **/

int parentWorker(){
    ssize_t bytesRead;

    /*Generate Random Num*/
    srandom(time(NULL));
    int num = random() % 1000000000;

    char buffer[256];
    sprintf(buffer, "A:%d-", num);
    printf("Parent: Num = %s \n", buffer);

    usleep(10000); //Wait creation of all children, JIC

    /*Close Unused Pipes*/
    for(int i = 0; i < N_PROC; i++){
        if(i != 0) close(pipefd[i][WRITE_END]);
        if(i != N_PROC - 1) close(pipefd[i][READ_END]);
    }

    /*Write to First Child*/
    if( (write(pipefd[0][WRITE_END], buffer, strlen(buffer)+1)) < 0 ){
        fprintf(stderr, "Error writing from parent : %s \n", strerror(errno));  
        exit(1);
    }

    memset(buffer, 0, 256);

    /*Read Response after Round Pipes*/
    if( (bytesRead = read(pipefd[N_PROC - 1][READ_END], buffer, 256)) < 0 ){ //Error Handling
       fprintf(stderr, "Error reading from Parent child response : %s \n", strerror(errno));  
       exit(1);

    } else if(bytesRead > 0){ //Print Result
        printf("\nParent: Received: %s \n\n" , buffer);
    
    } else { //If EOF Close pipes (read == 0)
        close(pipefd[0][WRITE_END]);
        close(pipefd[N_PROC - 1][READ_END]);
    }

    /*Wait for Children*/
    for(int i = 1; i < N_PROC; i++){
        waitpid(pid[i], NULL, WUNTRACED);
    }

    exit(0);
}