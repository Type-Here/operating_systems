/*
 ============================================================================
 Name        : pipe.c
 Author      : Man
 Version     :
 Copyright   : GPL 3.0
 Description : Hello World in C, Ansi-style
 ============================================================================
 Scrivere un programma C in cui:

- Il processo padre apre 2 pipe e crea un processo figlio
- Il padre apre un file e legge riga per riga il contenuto inviando la riga letta al processo figlio
- Il figlio elimina tutte le vocali dalla linea letta dalla pipe e invia la stringa risultante al padre
- Il padre stampa la stringa ricevuta e legge la prossima linea del file
- Il padre quando arriva all'EOF manda la stinga "%%%" e aspetta che il figlio termina
- Il figlio una volta che riceve la stringa chiude le pipe e termina restituendo al padre il numero di vocali rimosse

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> //write and others
#include <fcntl.h> //open
#include <sys/wait.h> //waitpid
#include <errno.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

#define FILE_TXT "file.txt"

int isVocal(char ch){
	return (ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u' || ch == 'y');
}

int main() {
	int pipeFdToPar[2];
	int pipeFdToCh[2];
	int child;

	printf("Welcome.\n\n");

	if ( (pipe(pipeFdToPar)) < 0 ){
		perror("Error Opening Pipe to Parent");
		exit(errno);
	}

	if ( (pipe(pipeFdToCh)) < 0 ){
		perror("Error Opening Pipe to Ch");
		exit(errno);
	}


	if( (child = fork()) < 0){ //Error fork
		perror("Error while Forking");
		exit(errno);

	} else if( (child > 0) ){ //I'm in Parent
		int fd, bytesRead, bytesWritten, lineSize = 0;
		char ch;
		char line[256] = {0};
		char response[256] = {0};
		int wstatus = 2;

		char endSignal[] = "%%%";

		close(pipeFdToPar[PIPE_WRITE]);
		close(pipeFdToCh[PIPE_READ]);

		//Open file to read from
		if( (fd = open(FILE_TXT, O_RDONLY, 0640)) < 0 ){
			fprintf(stderr, "Error while opening %s in Parent Ps : %s\n", FILE_TXT, strerror(errno));
			exit(errno);
		}

		//Read from file line by line
		while( (bytesRead=read(fd, &ch, 1)) > 0 ){
			if(ch != '\n' && ch != '\0' && lineSize < 256){

				line[lineSize++] = ch;

			} else {

				line[lineSize++] = '\n';
				line[lineSize++] = '\0';

				//Send to Child
				if( (bytesWritten = write(pipeFdToCh[PIPE_WRITE], line, lineSize)) < 0 ){
					fprintf(stderr, "Error while writing to child pipe, fd=%d: %s \n", pipeFdToCh[PIPE_WRITE], strerror(errno));
					exit(errno);
				}

				//Reset line buffer to add another line next cicle
				memset(line, '\0', sizeof(line));
				lineSize = 0;


				//Once sent to child wait for response in pipe (read from pipe)
				if ( (bytesRead = read(pipeFdToPar[PIPE_READ], response, 256)) > 0){
					printf("-Parent: Line without vocals: %s\n", response);
					memset(response, '\0', 256);

				} else if( bytesRead < 0){
					perror("Error in Parent while Reading from pipe");
					exit(errno);
				}

			}
		}

		if( (bytesRead < 0) ){
			fprintf(stderr, "Error while reading file %s : %s \n", FILE_TXT, strerror(errno));
			exit(errno);
		}

		//Send to Child endSignal '%%%' to Terminate Iteration
		if( (bytesWritten = write(pipeFdToCh[PIPE_WRITE], endSignal, sizeof(endSignal))) < 0 ){
			fprintf(stderr, "Error while writing endSignal '%%%%%%' to child pipe, fd=%d: %s \n", pipeFdToCh[PIPE_WRITE], strerror(errno));
			exit(errno);
		}


		close(fd);

		//Wait for child to end and return number of vocals removed
		if( (waitpid(child, &wstatus, WUNTRACED)) < 0 ){
			fprintf(stderr, "Error while waiting in parent ps fro child %d : %s", child, strerror(errno));
			exit(errno);
		}

		if (WIFEXITED(wstatus)) {
			printf("\n\texited, status=%d\n", WEXITSTATUS(wstatus));
		} else if (WIFSIGNALED(wstatus)) {
			printf("\n\tkilled by signal %d\n", WTERMSIG(wstatus));
		} else if (WIFSTOPPED(wstatus)) {
			printf("\n\tstopped by signal %d\n", WSTOPSIG(wstatus));
		} else if (WIFCONTINUED(wstatus)) {
			printf("\n\tcontinued\n");
		}

		printf("\tNum of Vocals removed = %d \n", WEXITSTATUS(wstatus));

	} else { //I'm in child

		close(pipeFdToPar[PIPE_READ]);
		close(pipeFdToCh[PIPE_WRITE]);

		int bytesReadCh = 0, bufSize = 0;
		char ch;
		char buffer[256] = {0};
		int numVocals = 0;

		//endSignal is %%%
		while(strcmp(buffer, "%%%") != 0 ){

			memset(buffer, '\0', 256);
			bufSize = 0;
			printf("Child: ");
			//Read a line from pipe (from Parent)
			while( (bytesReadCh = read(pipeFdToCh[PIPE_READ], &ch, 1)) > 0){
				printf("%c", ch);

				if(!isVocal(ch) ){
					buffer[bufSize++] = ch;
				} else {numVocals++;}

				if(ch == '\0') break;
			}
			if(bytesReadCh < 0){
				fprintf(stderr, "Error in Child %d while reading from pipe fd = %d: %s \n", getpid(), pipeFdToCh[PIPE_READ], strerror(errno));
				exit(errno);

			} else {
				printf("-Child:End Read Line from Pipe.\n");
			}

			// Write to Pipe from buffer (line without vocals)
			if( (write(pipeFdToPar[PIPE_WRITE], buffer, bufSize)) < 0 ){
				perror("Error in child while Writing in pipe");
				exit(errno);
			}

		}
		close(pipeFdToPar[PIPE_WRITE]);
		close(pipeFdToCh[PIPE_READ]);

		exit(numVocals);
	}

	printf("\nExiting..\n");

	return EXIT_SUCCESS;
}
