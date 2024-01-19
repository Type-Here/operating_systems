/**
 * Scrivere un Programma C
 * 
 * 1. Che cattura tutti i segnali e stampa il nome del segnale ricevuto
 * 
 * 2. Programma C che genera 4 Processi
 *    Dopo aver creato i processi, il padre a intervalli regolari, estrae a caso un intero compreso tra 0 e 3
 *    Manda un segnale al processo p[x] dove x è l'intero casuale.
 *    Il processo alla ricezione del segnale incrementa un contatore.
 *    Il primo processo che raggiunge il valore 10 stampa il proprio PID con accanto 'Ho Vinto'
 * 
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

#include <signal.h>

int counter;

void esercizio1();
void esercizio2();

void catchSignals(int sig){
    printf("Ho ricevuto Segnale %d : %s\n", sig, strsignal(sig));
}

void corsaDeiCavalliHandler(int sig){
    counter++;
}

void fatherTerm(int sig){
    printf("Sono Padre pid = %d. Esco \n", getpid());
    exit(0);
}

int main(int argc, char * argv[]){
    esercizio2();
    return 0;  
}


void esercizio1(){
        for(int i = 1; i < 32; i++){
        if(signal(i, catchSignals) == SIG_ERR){
            fprintf(stderr, "Errore Signal %d : %s \n", i, strerror(errno));
            //return 1;
        }
    }

    //Per Evitare la Busy Wait!
    while(1){
        if(pause() < 0){
            fprintf(stderr, "Errore Pause : %s \n", strerror(errno));
            //return 1;
        }
    }
}

void esercizio2(){
    srandom(time(NULL));
    pid_t pid[4];
    counter = 0;

    if( signal(SIGUSR1, corsaDeiCavalliHandler) == SIG_ERR){
        fprintf(stderr, "Errore Signal : %s \n", strerror(errno));
        //return 1;
    }

    for(int i = 0; i < 4; i++){
        if( (pid[i] = fork()) < 0 ){ //Error Handling
            fprintf(stderr, "Errore Creazione Figlio num %d : %s \n", i, strerror(errno));
            exit(1); 

        } else if(pid[i] == 0){ //Figlio
            printf("Creato figlio\n");

            while(1){
                if(pause() < 0) printf("Pause Interrotta\n");

                if(counter == 10){
                    printf("%d : Ho Vinto \n", getpid());

                    if(kill(getppid(), SIGTERM) < 0){
                        fprintf(stderr, "Errore Invio Segnale SIGTERM num %d : %s \n", i, strerror(errno));
                        exit(1);     
                    }
                    break;

                } else {
                    printf("PID %d: Sono al conteggio: %d \n", getpid(), counter);
                }
            }
            exit(0);
        
        } else { //Padre
        ;
        }
    }

    usleep(10000);
    printf("Padre %d\n", getpid());

    if(signal(SIGTERM, fatherTerm) == SIG_ERR){
        fprintf(stderr, "Errore Segnale SIGTERM in padre : %s \n", strerror(errno));
        exit(1); 
    }
    //Padre
    while(1){
        int a = random() % 4;
        if( kill(pid[a], SIGUSR1) < 0){
            fprintf(stderr, "Errore Invio Segnale a figlio num %d : %s \n", a, strerror(errno));
            exit(1);  
        }

        usleep(500000);
    }

    exit(0);
}