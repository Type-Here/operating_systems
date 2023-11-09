/*
 ============================================================================
 Name        : pipeNonBloccanti.c
 Author      : Man
 Version     :
 Copyright   : GPL 3.0
 Description : Hello World in C, Ansi-style
 ============================================================================
 * Programma C scrive in un file 100'000 interi scelti a caso.
 * Successivamente crea 4 figli. Ogni figlio precesserà 1/4 del File e, ( Variante Esercizio: pipeNonBloccanti.c: l'intero file )
 * tramite una pipe, indicherà al padre la posizione nel file di ogni intero multiplo rispettivamente:
 * 991, 1117, 2221, 3323
 * Il padre una volta terminati tutti i figli stamperà tutti i risultati ricevuti.
 * Si suggerisce di utilizzare la pipe2() con il flag O_NONBLOCK
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <ctype.h>

#define _GNU_SOURCE

#include <fcntl.h>
#include <unistd.h>

#include <errno.h>

#include <sys/stat.h>

#define FILENAME "intList.bin"
#define INTNUM 100000

#define READPIPE 0
#define WRITEPIPE 1

#define N_PROC 4

#define MAX 0
#if MAX == 0
	#define RANDOM() (random())
#elif MAX < 4
	#define RANDOM() (4)
#else
	#define RANDOM() (random() % (MAX))
#endif

struct posizione {

	off_t position;
	pid_t child;

};

int childWorker(int childPos, int divisor, int * pipefd, off_t offsetStart, off_t size);
int parentWorker(int *, int (*)[2]);
int getNumberFromFile(struct posizione *p, int fd); //fd should be open before execution


/** ---------------- MAIN --------------- **/

int main(void) {

	(void)&pipe;

	int divisori[] = {991, 1117, 2221, 3323};

	int pid[4];
	int pipefd[4][2];
	int i, fd;

	int sizeForEachChild[N_PROC];

	struct stat sb; //fstat struct

	srandom(time(NULL));

	/*Open FILENAME to write random generated integers*/
	if( (fd=open(FILENAME, O_CREAT | O_TRUNC| O_RDWR, 0640)) < 0 ){
		perror("Error while opening file");
		exit(EXIT_FAILURE);
	}

	/*Generate INTNUM number of random int and write them*/
	for(i = 0; i < INTNUM; i++){
		int randomNum = RANDOM();

		if( (write(fd, &randomNum, 4)) < 0 ){
			perror("Error while writing in file");
			exit(EXIT_FAILURE);
		}
	}

	/*Read file info using Syscall fstat (Used for sb.st_size)*/
	if( (fstat(fd, &sb)) < 0 ){
		perror("Error while executing stat");
		exit(1);
	}

	/*Close File*/
	close(fd);

	/* Calculate each Child offset in order to divide the parsing of the file
	 * in N_PROC parts all equals except maybe the last one (+ the remainder of the division) */
	sizeForEachChild[0] = sb.st_size / N_PROC;
	for(int i = 1; i < N_PROC - 1; i++){
		sizeForEachChild[i] = sizeForEachChild[0];
	}
	sizeForEachChild[N_PROC - 1] = sb.st_size - sizeForEachChild[0]*(N_PROC-1);

	/*Alternative Simpler Method WITHOUT fstat: because I know exactly how much I wrote*/
#if 0
	sizeForEachChild[0] = INTNUM * sizeof(int) / N_PROC;
		for(int i = 1; i < N_PROC - 1; i++){
			sizeForEachChild[i] = sizeForEachChild[0];
		}
	sizeForEachChild[N_PROC - 1] = INTNUM * sizeof(int) - sizeForEachChild[0]*(N_PROC-1);
#endif

	/*Create a Pipe for each future child: FLAG O_NONBLOCK, use of pipe2*/
	for(i = 0; i < N_PROC; i++){
		pipe2(pipefd[i], O_NONBLOCK);
	}

	/*Forking*/
	for(i = 0; i < N_PROC; i++){
		if( (pid[i] = fork()) < 0 ){ //Error Handling
			fprintf(stderr, "Error while creating child %d : %s \n", i, strerror(errno));
			exit(EXIT_FAILURE);

		} else if( pid[i] == 0 ){ // i-th Child

			/*Closed Unitilized pipes-end*/
			for(int j = 0; j < N_PROC; j++){
				close(pipefd[j][READPIPE]);
				if(i != j) close(pipefd[j][WRITEPIPE]);
			}

			/*Send to childWorker Function*/
			childWorker(i, divisori[i], pipefd[i], i*sizeForEachChild[i], sizeForEachChild[i]);
			exit(1); // Should never go here

		}
	}
	//Only Parent Should Arrive Here

	/*In Parent Process, close write-end of pipes*/
	for(i = 0; i < N_PROC; i++){
		close(pipefd[i][WRITEPIPE]);
	}

	/*Send to parentWorker*/
	parentWorker(pid, pipefd);

	return EXIT_SUCCESS;
}

/** ---------------- CHILD FUNCTION --------------- **/

int childWorker(int childPos, int divisore, int * pipefd, off_t offsetStart, off_t size){
	int buffer;
	int fd;
	ssize_t bytesRead;
	off_t counter = 0;

	/*Open File in Read Mode*/
	if( (fd=open(FILENAME, O_RDONLY, 0640)) < 0 ){
		perror("Error opening file in child");
		exit(EXIT_FAILURE);
	}

	/*Set starting offset with lseek syscall. */
	if( lseek(fd, offsetStart, 0) < 0 ){
		fprintf(stderr, "Error while setting start offset in child %d: %s \n", childPos, strerror(errno));
		exit(EXIT_FAILURE);
	}

	/*Printed Info*/
	printf("I'm Child %d: Start From %ld: Size: %ld \n", childPos, offsetStart, size);

	/* Reading each number from file*/
	while( (bytesRead = read(fd, &buffer, sizeof(int))) > 0){

		/*If number from file divisible for 'divisore' send to parent its offset position. */
		if(buffer % divisore == 0){
			off_t pos;

			/*Get offset with lseek syscall (move by 0 from current offset and return its value)*/
			if( (pos = lseek(fd, 0L, SEEK_CUR)) < 0 ){
				fprintf(stderr, "Error while getting position: %s \n", strerror(errno));
				exit(EXIT_FAILURE);
			}

			pos -= sizeof(int); /*IMPORTANTE!! sennò legge il numero dopo!*/

			/*Write in pipe to parent*/
			if( (write(pipefd[WRITEPIPE], &pos, sizeof(off_t)) < 0 ) ){
				//if(errno != EWOULDBLOCK && errno != EAGAIN){
					fprintf(stderr, "Error while writing from child %d to parent : %s \n", childPos, strerror(errno));
					exit(EXIT_FAILURE);
				//}
			}

			/*Counter for Stop after reaching Limit Size*/
			counter += bytesRead;
			if(counter >= size){
				bytesRead = 0;
				break;
			}
		}
	}

	/*Error Handling*/
	if(bytesRead < 0){
		perror("Error while reading in child");
		exit(EXIT_FAILURE);

	/*bytesRead == 0 : Job Done. Close Pipe and File*/
	} else {
		printf("Child %d: Number %d Terminated\n", getpid(), childPos);
		close(pipefd[WRITEPIPE]);
		close(fd);
	}

	exit(0);
}

/** ---------------- PARENT FUNCTION --------------- **/

int parentWorker(int * pid, int (*pipefd)[2]){

	int i, fd, numIndex = 0;
	int count = N_PROC;
	ssize_t bytesRead;
	int errno;

	/*Array of struct posizione where all offsets will be saved*/
	struct posizione numbers[INTNUM];

	/* Open File in read mode*/
	if( (fd=open(FILENAME, O_RDONLY, 0640)) < 0 ){
		perror("Error Opening file in parent in read_only mode");
		exit(EXIT_FAILURE);
	}

	/*Loop while children are alive*/
	while(count){
		off_t buffer;

		/*Read from each pipe*/
		for(i = 0; i < N_PROC; i++){

			if(pid[i] == 0) continue; //Ignore closed pipes

			if( (bytesRead = read(pipefd[i][READPIPE], &buffer, sizeof(off_t))) < 0){ /*Error Handling*/

				/*NON_BLOCKING ERROR from O_NONBLOCK FLAG from pipe!*/
				if(errno != EWOULDBLOCK && errno != EAGAIN){
					fprintf(stderr, "Error while reading from child %d : %s \n", i, strerror(errno));
				}
				else {
					printf("Child %d hasn't done yet\n", i);
				}

			} else if( bytesRead == 0 ) { /*This Child has terminated*/
				pid[i] = 0;
				--count;
			} else { /*I receive something from child*/
				numbers[numIndex].position = buffer;
				numbers[numIndex++].child = i;
			}
		}
		usleep(10000); // wait for 10ms
	}

	/*Print the Result*/
	int j;
	for(j = 0; j < numIndex; j++){
		printf("Num Found: %d - ", getNumberFromFile(&numbers[j], fd));
		printf("From Child: %d \n", numbers[j].child);
	}

	close(fd);

	return 0;
}

/* -- READ INT FROM BIN FILE AND OFFSETS FROM CHILDREN -- */

int getNumberFromFile(struct posizione * p, int fd){
	int num;
	if( (lseek(fd, p->position, 0)) < 0){
		fprintf(stderr, "Error in parent while getting number via lseek offset in file: %s \n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if( (read(fd, &num, sizeof(int))) < 0 ){
		fprintf(stderr, "Error while reading from file in parent: %s \n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	return num;
}

