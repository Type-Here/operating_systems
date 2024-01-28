/*
 ============================================================================
 Name        : testFork1.c
 Author      : Man
 Version     :
 Copyright   : Your copyright notice
 Description : Alternativa Esercizio 2 Lab SO
 =================================================
*
* Scrivere un programma C che crea N_PROC sottoprocessi.
* Ogni processo figlio stamperà gli interi nell'intervallo da "inizio" a "fine"
* ognuno preceduto dal proprio pid.
* Il processo padre attenderà la fine di tutti i suoi figli e poi uscirà.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#define N_PROC 10

void conta(int start, int end){
	if(start > end) {
		fprintf(stderr, "Errore dai in input: %s", __func__);
		return;
	}

	int i = start;
	int offset = 0;
	int id = getpid();
	char buffer[1024];

	for(; i < end; i++){
		offset += snprintf(buffer + offset, 1024 - offset, "id: %d -> %d\n", id, i);
		//printf("id: %d -> %d\n", id, i);
		//printf("%s",buffer);
	}
	write(STDOUT_FILENO, buffer, offset);
	//printf("%s", buffer);
}

int creaFigliEConta(int start, int end, pid_t * childpids, int index){
	//int child;
	//int wstatus = 0;

	if( ( childpids[index] = fork() ) == 0 ){

		conta(start, end);
		exit(EXIT_SUCCESS);

	} else if( childpids[index] > 0 ) {

		//waitpid(child, &wstatus, WUNTRACED);
		//printf("CHILD %d\n", childpids[index]);

	} else {

		fprintf(stderr, "Unable to create Child ");
	}

	return 0;
}

int main(){

	int i, j;
	pid_t childpids[N_PROC];
	int wstatuses[N_PROC];

	printf("--- INIZIO PROGRAMMA ---\n");

	printf("- PID Padre: %d\n\n", getpid());

	for(i = 0; i < N_PROC; i++){
		creaFigliEConta(i*10,(i+1)*10, childpids, i);
	}

	//Sono il padre e aspetto tutti i figli
	for(j = 0; j < N_PROC; j++){
		waitpid(childpids[j], &wstatuses[j], WUNTRACED);
	}

	for(j = 0; j < N_PROC; j++){
		printf("-%d Child PID: %d with Exit status %d\n", j, childpids[j], WEXITSTATUS(wstatuses[j]));
	}

	printf("\n--- TERMINO IL PROGRAMMA ---\n");

	return EXIT_SUCCESS;
}
