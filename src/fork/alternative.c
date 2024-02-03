/*
 ============================================================================
 Name        : alternative.c
 Author      : Man
 Version     :
 Copyright   : Your copyright notice
 Description : Alternativa Esercizio 2 Lab SO
 ============================================================================
 *
 * Versione Alternativa *
 *
 1) Progetto Fork(): Il padre fa cose, il figlio un'altra.
 2) Programma che esegue ls.
 3) Programma che esegue ls e cattura output in un file -> (Uso delle pipe).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

#define FINALOUTPUT "file2.txt"
#define MAXLINE 4096

#define READ_END 0
#define WRITE_END 1

int writeWith2Sons(){
		char ls [] = "/usr/bin/ls";
		char * const chArgsLs[] = {ls, "-l", NULL};

		char tee [] = "/usr/bin/tee";
		char * const chArgsTee[] = {tee, FINALOUTPUT, NULL};

		//char childOutput [1024];

		pid_t child1, child2;

		int wstatus1, wstatus2;

		int pipefd[2];

		if( (pipe(pipefd)) < 0 ){
			fprintf(stderr, "Failed to Open Pipe. \n");
			exit(1);
		}

		// Apro Child 1 e Leggo con ls
		if( (child1 = fork()) == 0 ){ //Sono in Child 1
			close(pipefd[READ_END]); //Chiudo Pipe in Lettura
			dup2(pipefd[WRITE_END], STDOUT_FILENO);
			close(pipefd[WRITE_END]); //Chiudo Pipe in Scrittura

			if( (execv(ls, chArgsLs) < 0 ) ) {
				fprintf(stderr, "Error: cannot execv %s\n", ls);
				exit(1);
			}

		} else if(child1 > 0){ //Parent

			waitpid(child1, &wstatus1, 0);
			printf("\nChild: %d, Waitpid WSTATUS = %d\n", child1, wstatus1);

		} else { //Error Handling

			close(pipefd[READ_END]); // Chiudo Pipe in Lettura
			close(pipefd[WRITE_END]); // Chiudo Pipe in Scrittura
			fprintf(stderr, "Failed to Fork \n");
			exit(1);
		}

		//Parent

		printf("\nOra Eseguo il child2\n");

		// Apro Child 2 e Scrivo il risultato di ls
		if( (child2 = fork()) == 0 ){
			close(pipefd[WRITE_END]); //Chiudo Pipe in Lettura
			//close(pipefd[READ_END]); //Chiudo Pipe in Scrittura

			if( (execv(tee, chArgsTee)) < 0 ){
				fprintf(stderr, "Error cannot exec %s", tee);
				exit(1);
			}

		} else if(child2 > 0){ //Parent

			waitpid(child2, &wstatus2, WUNTRACED);
			printf("\nChild: %d, Waitpid WSTATUS = %d\n", child2, wstatus2);

		} else {

			close(pipefd[READ_END]); // Chiudo Pipe in Lettura
			close(pipefd[WRITE_END]); // Chiudo Pipe in Scrittura
			fprintf(stderr, "Failed to Fork \n");
			exit(1);
		}

		return 0;
}

/*
int main(){
	printf("Entro in esecuzione. \n");

	writeWith2Sons();

	printf("--FINE--\n");

	return EXIT_SUCCESS;
}*/
