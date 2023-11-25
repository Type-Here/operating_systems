/**
 * Scrivere un Programma C:
 * Legge dalla Linea di Comando la dimensione totale di memoria (TotalSize) 
 * da allocare attraverso 3 matrici quadrate (n x n -> n = sqrt( (TotalSize * 1024 * 1024 / (3 *4)) ) )
 * e un intero che stabilisce il valore di  NumOps
 * 
 * Una volta allocate le matrici, inizializzare le matrici A e B con i valori casuali compresi tra -10 e +10
 * 
 * Una volta allocate le 3 matrici, si effettuino un numero di operazioni pari a NumOps.
 * Ogni operazione consiste nel selezionare un elemento a caso della matrice A e della matrice B,
 * effettuare la moltiplicazione ed assegnare il risultato ad un elemento della matrice C:
 * C[i][j] += A[k][l] * B[m][o]
 * 
 * Al termine del programma stamperà il numero di operazioni al secondo:
 * TempoTotale/NumOps
 * 
*/
#include "../libso/src/ourhdr.h"

#include <stdio.h>
#include <stdlib.h>

#include <math.h>
#include <time.h>

#define NUMOPS 10000

int main(int argc, char * argv[]){

    if(argc < 2){
        fprintf(stderr, "Usage %s TotalSize \n", argv[0]);
        exit(1);
    }

    printf("Indirizzo argc: %p \nIndirizzo argv[0]:%p \n", &argc, argv);

    srandom(time(NULL));

    int TotalSize = atoi(argv[1]);
    int n = sqrt((TotalSize * 1024 * 1024)/(3*4));

    int ** A = calloc(sizeof(int*), n);
    int ** B = calloc(sizeof(int*), n);
    int ** C = calloc(sizeof(int*), n);

    for(int i = 0; i < n; i++){
        A[i] = calloc(sizeof(int), n);
        B[i] = calloc(sizeof(int), n);
        C[i] = calloc(sizeof(int), n);
    }

    for(int i = 0; i < n; i++){
        for(int j = 0; j < n; j++){
            A[i][j] = -10 + (random() % 21);
            B[i][j] =  -10 + (random() % 21);
        }
    }

    startCounting();

    for(int z = 0; z < NUMOPS; z++){
        int i,j,k,l,m,o;

        i = random() % n;
        j = random() % n;
        k = random() % n;
        l = random() % n;
        m = random() % n;
        o = random() % n;

        C[i][j] += A[k][l] * B[m][o];
    }

    stopCounting();
    double totalTime = getRealTime();
    printResourceUsage(RUSAGE_SELF);
    printf("\nTotal Time = %.3lf msec; \nNumOps = %d \n", totalTime, NUMOPS);
    printf("Ops/Sec = %.3lf \n\n", NUMOPS * 1000 / (totalTime));

    for(int i = 0; i < n; i++){
        free(A[i]);
        free(B[i]);
        free(C[i]);
    }

    free(A); 
    free(B); 
    free(C);

    return 0;
}
