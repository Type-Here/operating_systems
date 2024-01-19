/* 3. Programma che controlla i riflessi dell'utente:
 *    Genera 3 - 4 simboli al secondo ma quando stampa O (lettera) l'utente deve reagire lanciando segnale
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
int valid = 0;
int points = 0;

void handler(){
    if(valid) points++;
}

int main(){
    srandom(time(NULL));
    int counter = 0;
    
    if(signal(SIGINT, handler) == SIG_ERR){
        fprintf(stderr, "Errore Signal : %s \n", strerror(errno));
        exit(1);
    }
    
    while(counter < 10){
        int num = random() % 100;
        if(num < 20){
            valid = 1;
            printf("O\n");
            
        } else { //Volendo Usa Write per syscall
            valid = 0;
            if(num < 39) printf("K\n");
            else if(num < 70) printf("Z\n");
            else printf("0\n");
        }

        usleep(500000);
        counter++;
    }

    printf("Il tuo Punteggio %d \n", points);

    return 0;
}