/*
 ============================================================================
 Name        : mylscounter.c
 Author      : Man
 Version     : 0.1
 Copyright   : GPL 3.0
 Description : Create a child process to read a ls 'path' from argv then parent ps read all files from dir and print number of '\n' in all files
 ============================================================================
 */

/*NB Ho aggiunto ad eclipse l'argomento: './test' come path per testare il programma!!*/


#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h> //for strerror  (and strtok in this case)
#include <unistd.h> //pipe, fork, read, close
#include <sys/wait.h> //waitpid
#include <fcntl.h> //open

#define PIPE_READ 0
#define PIPE_WRITE 1

#define EXPECTED_VALUE 5 //for testing purposes

int countNewLines(char * path, char * buffer){
	int number = 0;

	//Sistemo Path per leggere cartella corretta
	char correctedPath[512];
	strcpy(correctedPath, path);
	if( *(path + strlen(path) - 1) != '/') strcat(correctedPath, "/");
	strcat(correctedPath, buffer);

	printf("In func %s: \n\tbuffer: %s \n\tpath: %s \n\tcorrectedPath: %s \n\n", __func__, buffer, path, correctedPath);

	char fileChar;
	int fd, bytesRead;

	if( (fd = open(correctedPath, O_RDONLY, 0640)) > 0 ){
		printf("Opened: fd=%d, %s\n\n", fd, correctedPath);
		while( (bytesRead = read(fd, &fileChar, 1)) > 0 ){
			if(fileChar == '\n') ++number;
			//printf("Ho letto: %c\n", fileChar);
		}
		if(bytesRead < 0){
			fprintf(stderr,"Error Reading Buffer from file %s \n: %s", correctedPath, strerror(errno));
			exit(errno);
		}

	} else {
		fprintf(stderr, "Error Opening file: %s", correctedPath);
		perror(":");
		return 0;
	}

	close(fd);

	return number;
}

int main(int argc, char * argv[]) {
	printf("Welcome.\n\n");
	int pipefd[2];
	int child;
	char ls[] = "/usr/bin/ls";

	if( argc < 2 ){
		fprintf(stderr, "Num args no valid:\n Usage: %s [path]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if( (pipe(pipefd)) < 0 ){
		perror("Error opening pipe. \n");
		exit(errno);
	}

	if( (child = fork()) == 0 ){ //Sono nel figlio
		close(pipefd[PIPE_READ]);

		if( (dup2(pipefd[PIPE_WRITE], STDOUT_FILENO) < 0) ){
			perror("Error dup2 file. \n");
			exit(errno);
		}

		argv[0] = ls;

		/*for(int i = 0; i < argc; i++){
			printf("\n - argv[%d] = %s\n", i, argv[i]);
		}*/

		if( (execv(ls,argv)) < 0 ){
			perror("Error in execv\n");
			exit(errno);
		}

		exit(1); //Giusto per sicurezza
	}

	else if( (child > 0) ){ //Sono nel padre

		int wstatus;
		int numOfLines = 0;
		int bytesRead;

		waitpid(child, &wstatus, WUNTRACED);
		if(WEXITSTATUS(wstatus)){
			fprintf(stderr, "Error in Child Process PID %d : %s", child, strerror(WEXITSTATUS(wstatus)));
			exit(WEXITSTATUS(wstatus));
		}

		close(pipefd[PIPE_WRITE]);

		char buffer[256];

		printf("I'm back in parent ps:\n\n");

		while( (bytesRead = read(pipefd[PIPE_READ], buffer, 256)) > 0 ){
			char * token;
			token = strtok(buffer, "\n");
			while( token != NULL ) {
				printf("File considered: %s\n", token);
				numOfLines += countNewLines(argv[1], token);
			    token = strtok(NULL, "\n");
			}

		}
		if(bytesRead < 0){
			perror("Error Reading Buffer from Pipe\n");
			exit(errno);
		}

		printf("\tTotal number of \\n in file in directory = %d\n", numOfLines);
		printf("\tExpected from test files: %d\n\n", EXPECTED_VALUE);

		close(pipefd[PIPE_READ]);

		printf("Exiting... \n");
	}

	else { //Error in child creation
		perror("Fork Failed\n");
		exit(errno);
	}



	return EXIT_SUCCESS;
}
