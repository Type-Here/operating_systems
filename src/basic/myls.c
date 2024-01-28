/*
 ============================================================================
 Name        : myls.c
 Author      : Man
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 * Quesito 1) Scrivere un programma C che crea un sottoprocesso che per eseguire il comando 'ls'.
 * Il programma deve visualizzare l'output di ls e stamperà l'exit status del figlio dopo la terminazione del figlio.
 * Tutti i parametri passati sulla linea di comando dovranno essere passati a ls.
 *
 * Quesito 2) Il programma precedente deve ridirigere l'output del comando ls nel file ./output.txt anziché nelle standard output.
 *
 * HINT: indicare il percorso completo di ls, per trovare il percorso usare il comando shell 'which ls'
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
//#include <ourhdr.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

#define OUTPUT "./output.txt"

int childLs( int argc, char * argv[]){
	int child;
	int wstatus;

	char ls[] = "/usr/bin/ls";

	char ** argvLs = malloc(sizeof(char *) * (argc+1));
	argvLs[0] = ls;
	argvLs[argc] = NULL;

	int i;
	for(i = 1; i < argc; i++){
		argvLs[i] = argv[i];
	}


	if( (child = fork()) == 0 ){ //Child

		printf("Sono in child. Eseguo ls:\n");

		int fd;

		if( (fd=open(OUTPUT, O_CREAT | O_WRONLY | O_TRUNC, 0640)) < 0 ){
			fprintf(stderr, "Errore Apertura File Output in %s\n", OUTPUT);
		}

		if( (dup2(fd, STDOUT_FILENO)) < 0 ){
			fprintf(stderr, "Errore dup2 \n");
		}

		/*printf("\nfd di %s: %d \nfd stdout: %d \nnewfd: %d\n", output, fd, STDOUT_FILENO, newfd);*/

		if( (execv(ls, argvLs)) < 0 ){
			fprintf(stderr, "Errore Exec\n");
		}

	} else if(child > 0){ //Parent

		waitpid(child, &wstatus, WUNTRACED);
		free(argvLs);

	} else { //Error Handling
		fprintf(stderr, "Errore Fork.\n");
	}

	return wstatus;
}

int main(int argc, char * argv[]) {
	printf("\nNBenvenuti \n");

	int wstatus = childLs(argc, argv);

	printf("\nExit Status Child = %d\n", WEXITSTATUS(wstatus));

	return EXIT_SUCCESS;
}
