/*
 ============================================================================
 Name        : lab2.c
 Author      : Man
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 1) Progetto Fork(): Il padre fa cose, il figlio un'altra.
 2) Programma che esegue ls.
 3) Programma che esegue ls e cattura output in un file -> (Uso delle pipe).
 */

#include <fcntl.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define FINALOUTPUT "file.txt"
#define MAXLINE 4096


/*   --------------------------   */

#include <errno.h>    /* for definition of errno */
#include <stdarg.h> /* ANSI C header file */
#include <limits.h>


/* --------------------------------------------------------- */
int avviaLs(){

	char ls [] = "/usr/bin/ls";
	char * const chArgs[] = {ls, "-l", NULL};

	pid_t child;
	pid_t childpid;

	int wstatus;

    if( (child = fork()) == 0 ) { //child
        printf("I'm in child.\n\n");

        if( (execv(ls, chArgs) < 0 ) ) {
        	printf("Error: child: %d exec failed\n", child);
        	fprintf(stderr, "Error: cannot execv %s\n", ls);
        	exit(EXIT_FAILURE); //must exit child
        }
        //exit(0);
    }

    else if( child > 0 ) { //child>0 I'm no child, I'm parent, wait for child; child = child PID

        childpid = waitpid(child, &wstatus, 0);
        printf("\n\tFine Attesa Child PS %d\n", childpid);
        printf("\nChild: %d, Waitpid WSTATUS = %d\n", child, wstatus);
    }

    else { //child < 0, error
        printf("Error: child fork failed\n");
        exit(EXIT_FAILURE);
    }

	return 0;
}

int avviaLsScrivi(){
	char ls [] = "/usr/bin/ls";
	char * const chArgsLs[] = {ls, NULL};

	char childOutput [1024];

	pid_t child;
	pid_t childpid;

	int wstatus;

	int pipefd[2];

	if ( (pipe(pipefd) < 0) ){
		fprintf(stderr, "Failed to Open Pipe. \n");
		exit(EXIT_FAILURE);
	}

	if ( (child = fork()) == 0){ //Sono nel figlio
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[0]); //Chiudo la lettura
		close(pipefd[1]); //Chiudo la scrittura



		if( (execv(ls, chArgsLs)) < 0 ){
			//printf("Errore in Esecuzione Child Exec: %s\n", ls);
			fprintf(stderr, "Exec in Child Failed; %s\n", ls);
			exit(EXIT_FAILURE);
		}
		//exit(1);
	}

	else if( child > 0 ){ //Sono nel padre
		int bytesRead = 0; //bytes letti da pipe figlio
		int fdw; //file descriptor of the write file

		childpid = waitpid(child, &wstatus, 0);
		printf("\n\tFine Attesa Child PS %d\n", childpid);

		close(pipefd[1]); //Chiudo la scrittura

		//Se il figlio è uscito con errore, esci
		if(wstatus != 0){
			printf("Child non ha eseguito correttamente il codice. Esco.. \n\n");
			exit(WEXITSTATUS(wstatus));
		}

		//Leggo l'output dal pipe figlio in un array
		if( ( bytesRead = read(pipefd[0], childOutput, sizeof(childOutput)) ) > 0 ){
			childOutput[bytesRead] = '\0';

		} else {
			printf("Errore in lettura buffer da pipe %d, in modalità lettura. \n", pipefd[0]);
			fprintf(stderr, "Cannot write in array from pipe %d. PID=%d", pipefd[0], getpid());
			exit(EXIT_FAILURE);
		}

		// -- Scrivo l'array su file --

		close(pipefd[0]); // Chiudo la lettura

		if( ( fdw = open(FINALOUTPUT, O_WRONLY | O_RDONLY | O_CREAT | O_TRUNC, 0640)) < 0 ){
			fprintf(stderr, "Cannot Open File %s\n", FINALOUTPUT);
			exit(EXIT_FAILURE);
		}

		if( ( write(fdw, childOutput, bytesRead + 1) ) < 0){
			printf("Errore in %s. Uscita.\n", __func__);
			fprintf(stderr, "Cannot Write %d\n", bytesRead+1);
			exit(EXIT_FAILURE);
		}

		close(fdw);

	}

	return 0;
}

/*
int main(void) {
	printf("\nPRIMO TASK: \nEseguo un Figlio con ls.\n\n");
	avviaLs();

	printf("\n\nSECONDO TASK: \nEseguo pipe e figlio esegue ls, padre salva in file\n\n");
	avviaLsScrivi();

	printf("Operazione Conclusa. Grazie e Arrivederci.\n");

	return EXIT_SUCCESS;
}*/
