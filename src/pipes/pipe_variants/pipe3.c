/*
 ============================================================================
 Name        : pipe3.c
 Author      : Man
 Version     : To be completed. Works already for Basic Cases but not all.
 Copyright   : GPL 3.0
 Description : Upper-Intermediate Level
 ============================================================================
 NB: Da Finire. Al momento funziona nei casi base ma non è completo.
 Scrivere un programma C in cui:

- Gestisci il file in base alla taglia. Se minore di 1MB leggi tutto in una volta, altrimenti in chunk
- Il processo padre apre 2 pipe e crea un processo figlio
- Il padre apre un file e legge riga per riga il contenuto inviando la riga letta al processo figlio
- Il figlio elimina tutte le vocali dalla linea letta dalla pipe e invia la stringa risultante al padre
- Il padre stampa la stringa ricevuta e legge la prossima linea del file
- Il padre quando arriva all'EOF, aspetta che il figlio termina
- Il figlio una volta finito di elaborare l'ultima riga, chiude le pipe e termina restituendo al padre il numero di vocali rimosse

 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> //write and others
#include <fcntl.h> //open
#include <sys/wait.h> //waitpid
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define PIPE_READ 0
#define PIPE_WRITE 1
#define MAX_SIZE_ONESHOT 1048576 // 1MB Arbitrarily Chosen
#define CHUNKSIZE 256 // Arbitrarily Chosen
#define FILE_TXT "file.txt"

int parentWorker(const char * file, int pipes_fd[][2], pid_t child);
int childWorker(int pipes_fd[][2]);

int readChunkFromFile(int fd, pid_t child, int pipes_fd[][2]);
int readOneShotFromFile(int fd, pid_t child, int pipes_fd[][2], off_t st_size);

int sendToChild(char * readch, int bytesRead, pid_t child, int pipes_fd[][2]);
int readFromChild(int pipes_fd[][2]);

int isVowel(char ch){
	return (ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u' || ch == 'y');
}

int main(int argc, char * argv[]) {
    char * file;
    if(argc < 2){
        printf("No argument found, using default: %s \n\n", FILE_TXT);
        file = FILE_TXT;
    } else {
        file = argv[1];
    }


	int pipes_fd[2][2];
	pid_t child;   

    // Opening Pipes
    for(int i = 0; i < 2; i++){
        if ( (pipe(pipes_fd[i])) < 0 ){
		    fprintf(stderr, "Error Opening Pipe num %d : %s \n", i, strerror(errno));
		    exit(EXIT_FAILURE);
	    }
    }

    // Fork

    if( (child = fork()) < 0 ){ //Error Handling
        fprintf(stderr, "Error Fork : %s \n", strerror(errno));
		exit(EXIT_FAILURE);
    
    } else if (child > 0){ //Parent
        parentWorker(file, pipes_fd, child);

    } else { // Child
        childWorker(pipes_fd);
        exit(EXIT_FAILURE); //Should not get here, JIC
    }

    printf("\nExiting..\n");
    return 0;
}


/**
 * Parent Worker
*/
int parentWorker(const char * file, int pipes_fd[][2], pid_t child){
        int fd, wstatus;
        struct stat sb;

        close(pipes_fd[0][PIPE_READ]); //Parent will write in pipe 0
        close(pipes_fd[1][PIPE_WRITE]); //Parent will read in pipe 1

        //Open file to read from
		if( (fd = open(file, O_RDONLY, 0640)) < 0 ){
			fprintf(stderr, "Error while opening %s in Parent : %s\n", file, strerror(errno));
			exit(errno);
		}

        //Get File info (size)
        if(fstat(fd, &sb) < 0){
            fprintf(stderr, "Error in stat from parent : %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }   

        /* If file is greater than size chosen make multiple read from file.
         * Otherwise read all in one shot */
        if(sb.st_size > MAX_SIZE_ONESHOT){ 
            printf("File greater than %d KB: Will be read in chunks\n", MAX_SIZE_ONESHOT/1024);
            readChunkFromFile(fd, child, pipes_fd);
        } else {
            printf("File less than %d KB: Will be read only once\n", MAX_SIZE_ONESHOT/1024);
            readOneShotFromFile(fd, child, pipes_fd, sb.st_size);
        }

        close(fd);

        //Wait for child to end and return number of vocals removed
		if( (waitpid(child, &wstatus, WUNTRACED)) < 0 ){
			fprintf(stderr, "Error while waiting in parent ps fro child %d : %s", child, strerror(errno));
			exit(errno);
		}

		if (WIFEXITED(wstatus)) {
		    printf("\tNum of Vocals removed = %d \n", WEXITSTATUS(wstatus));
        } else {
            printf("Child ended with an error \n");
        }
}


/**
 * Child Worker
*/
int childWorker(int pipes_fd[][2]){

    int numVowels = 0, bytesRead;
        char buffer[CHUNKSIZE];

        close(pipes_fd[0][PIPE_WRITE]); //Child will read in pipe 0
        close(pipes_fd[1][PIPE_READ]); //Child will write in pipe 1

        while( (bytesRead = read(pipes_fd[0][PIPE_READ], buffer, CHUNKSIZE)) > 0){
            char response [CHUNKSIZE];
            int resp_size = 0;
            
            for(int i = 0; i < bytesRead; i++){
                if(isVowel(buffer[i])) continue;
                response[resp_size++] = buffer[i];
            }

            // Write to Pipe from buffer (line without vocals)
			if( (write(pipes_fd[1][PIPE_WRITE], response, resp_size)) < 0 ){
				fprintf(stderr, "Error in child while Writing in pipe: %s \n", strerror(errno));
				exit(EXIT_FAILURE);
			}
        }

        if(bytesRead < 0){ // Error Handling
            fprintf(stderr, "Error reading from child from pipe : %s \n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        //Else bytesRead == 0 : Closed Pipe
        close(pipes_fd[0][PIPE_READ]);
        close(pipes_fd[1][PIPE_WRITE]);


        exit(numVowels);
}


/**
 * Read in Chunk Bytes from File
*/
int readChunkFromFile(int fd, pid_t child, int pipes_fd[][2]){
    int remnants = 0;
    ssize_t bytesRead;

    char readch[2*CHUNKSIZE] = {0};
    char chunk[CHUNKSIZE] = {0};
    
    //Read from file in chunk and send line by line
		while( (bytesRead = read(fd, chunk, CHUNKSIZE)) > 0 ){
            strcat(readch, chunk);
            remnants = sendToChild(readch, bytesRead, child, pipes_fd);
            memset(readch, '\0', 2*CHUNKSIZE);
            
            if(remnants != 0){
                strncpy(readch, chunk + remnants, remnants);
            }

            memset(chunk, '\0', CHUNKSIZE);
        }

        if( (bytesRead < 0) ){ //Error Handling
			fprintf(stderr, "Error while reading file: %s \n", strerror(errno));
			exit(EXIT_FAILURE);

		} else { //BytesRead == 0 - EOF
            close(pipes_fd[0][PIPE_WRITE]);
        }
}

/**
 * Read all file altogether then parse it
*/
int readOneShotFromFile(int fd, pid_t child, int pipes_fd[][2], off_t st_size){
    int remnants = 0;
    ssize_t bytesRead;

    char fileRead[st_size + 1];

    if( (bytesRead = read(fd, fileRead, st_size)) < 0){
        fprintf(stderr, "Error while reading one shot from file : %s \n", strerror(errno));
		exit(EXIT_FAILURE);
    }
    
    sendToChild(fileRead, bytesRead, child, pipes_fd);
    close(pipes_fd[0][PIPE_WRITE]);
}


/**
 * Send Line to Child
 * @return number of char that where not sent to child (no new line encoutered till the end) 
*/
int sendToChild(char * readch, int bytesRead, pid_t child, int pipes_fd[][2]){
    int i = 0, bytesToSend = 0;
    char * sendFrom = readch;

    while(i < bytesRead){
        bytesToSend++;
        if(readch[i] == '\n'){
            
            //Send to Child
			if( (write(pipes_fd[0][PIPE_WRITE], sendFrom, bytesToSend)) < 0 ){
				fprintf(stderr, "Error while writing to child from pipe: %s \n", strerror(errno));
				exit(EXIT_FAILURE);
			}

			//Update base char and reset number of bytes to send
			sendFrom = &readch[i];
            bytesToSend = 0;
            
            //Once sent line to child wait for response
            readFromChild(pipes_fd);
        }

        i++; 
    }
    
    return bytesRead - bytesToSend;
}


/**
 * Read line from child without vowels
*/
int readFromChild(int pipes_fd[][2]){
    char response[CHUNKSIZE];
    ssize_t bytesRead = 0;

    //Once sent to child wait for response in pipe (read from pipe)
    if ( (bytesRead = read(pipes_fd[1][PIPE_READ], response, CHUNKSIZE)) > 0){
        printf("-Parent: Line without vowels: %s\n", response);

    } else if( bytesRead < 0){
        fprintf(stderr, "Error in Parent while Reading from pipe \n");
        exit(EXIT_FAILURE);
    }
    
    return 0;
}