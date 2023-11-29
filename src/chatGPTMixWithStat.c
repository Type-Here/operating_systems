/*
 ============================================================================
 Name        : chatGPTMixWithStat.c
 Author      : Man
 Version     :
 Copyright   : GPL 3.0
 Description : Hello World in C, Ansi-style
 ============================================================================
 * Scrivere un programma in linguaggio C che utilizza diverse system call,
 * tra cui fork, pipe, open, close, write, read, stat, lseek, exec, e wait.
 *
 * Il programma deve prendere come argomento il percorso di un file.
 * Il processo padre dovrà creare un processo figlio, aprire il file specificato,
 * calcolare la sua lunghezza utilizzando lseek o stat, inviare la lunghezza al processo padre attraverso una pipe,
 * quindi eseguire un nuovo programma (ad esempio, cat) nel processo figlio per stampare il contenuto del file.
 *
 * Nel frattempo, il processo padre dovrà leggere la lunghezza del file dalla pipe e stamparla.
 *
 * Suggerimenti:
 * Utilizzare la system call pipe per creare una pipe.Forkare il processo per creare un processo figlio.
 * Nel processo figlio, aprire il file specificato dal genitore e calcolare la lunghezza del file.Utilizzare la system call write
 * per inviare la lunghezza al processo padre attraverso la pipe.
 * Nel processo figlio, eseguire un nuovo programma (es. cat) utilizzando la system call exec.
 * Nel processo padre, utilizzare la system call read per leggere la lunghezza dal processo figlio attraverso la pipe.
 * Stampa la lunghezza del file nel processo padre.
 * Assicurarsi di gestire gli errori correttamente e chiudere i descrittori di file appropriati.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include <errno.h>

#define READ_END 0
#define WRITE_END 1

#define CAT "/usr/bin/cat"

int main(int argc, char * argv[]){

	pid_t pid;
	int pipefd[2];

	if(argc < 2){
		fprintf(stderr, "Usage %s path_to_file' \n", argv[0]);
	}

	if(pipe(pipefd) < 0){
		fprintf(stderr, "Errore Apertura Pipe : %s \n", strerror(errno));
	}

	if( (pid = fork()) < 0 ){ //Error Handling
		fprintf(stderr, "Errore Apertura Pipe : %s \n", strerror(errno));
		exit(1);

	} else if(pid == 0){ //Child
		close(pipefd[READ_END]);

		struct stat sb;

		if( stat(argv[1], &sb) < 0){
			fprintf(stderr, "Errore Esecuzione Stat : %s \n", strerror(errno));
			exit(1);
		}

		if(write(pipefd[WRITE_END], &sb.st_size, sizeof(off_t)) < 0){
			fprintf(stderr, "Errore Scrittura su Pipe da Figlio : %s \n", strerror(errno));
			exit(1);
		}

		char * childargs[] = {CAT, argv[1], NULL};
		printf("In Child: \n");
		printf("Size File: %ld \nDevId: 0x%lx \nDim in Blocchi: %ld\n", sb.st_size, sb.st_dev, sb.st_blocks);
		printf("Ultima Modifica: %s\n\n", ctime(&sb.st_mtim.tv_sec) );
		execve("/usr/bin/cat", childargs , NULL);
		exit(1); //Only if Exec Fails

	} else { // Parent
		close(pipefd[WRITE_END]);
		int bytesRead;
		int wstatus;
		off_t sizeRead;

		if( ( bytesRead = read(pipefd[READ_END], &sizeRead, sizeof(off_t)) ) < 0 ){
			fprintf(stderr, "Errore Lettura da Pipe da Padre : %s \n", strerror(errno));
			exit(1);
		}

		waitpid(pid, &wstatus, WUNTRACED);
		printf("\nFrom Parent: Size of File %ld \n", sizeRead);

		exit(0);
	}



	return EXIT_SUCCESS;
}
