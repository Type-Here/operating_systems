/* Programma C:
* Padre legge 1024 byte alla volta da un file mandato in argomento al programma.
* Manda i byte con pipe al figlio
* Figlio esegue grep -n -i -e qu[ioa]
* Figlio scrive su file result.txt no su stdout
* Padre attende vhe il figlio finisca
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <errno.h>

#define WRITE_END 1
#define READ_END 0

#define GREP "/usr/bin/grep"
#define RES_TXT "result.txt"
#define SIZE 1024

int main(int argc, char * argv[]){
    
    if( (argc < 2) ){
        fprintf(stderr, "Usage %s /path/to/file \n", argv[0]);
        exit(1);
    }

    int pipefd[2];
    int pid;

    /*Create Pipe*/
    if( (pipe(pipefd)) < 0){ 
        fprintf(stderr, "Error creating pipe : %s \n", strerror(errno));
        exit(1);
    }

    if((pid = fork()) < 0){ //Error Handling
        fprintf(stderr, "Error creating child with fork : %s \n", strerror(errno));
        exit(1);

    } else if(pid > 0){ //PARENT
        
        char buffer[SIZE];
        int fd;
        ssize_t bytesRead; 

        if( (fd = open(argv[1], O_RDONLY)) < 0 ){
            fprintf(stderr, "Error opening file %s : %s \n", argv[1], strerror(errno));
            exit(1);
        }

        while( (bytesRead = read(fd, buffer, SIZE)) > 0 ){
            
            if( (write(pipefd[WRITE_END], buffer, SIZE)) < 0){
                fprintf(stderr, "Error writing to pipe from Parent : %s \n", strerror(errno));
                exit(1);
            }
        }

        if(bytesRead  < 0){ //Error Handling
            fprintf(stderr, "Error reading from file %s : %s \n", argv[1], strerror(errno));
            exit(1);
            
        } else { //EOF
            close(fd);
            close(pipefd[WRITE_END]);
            waitpid(pid, NULL, WUNTRACED);
        }

        exit(0);

    } else { // pid == 0 CHILD
        close(pipefd[WRITE_END]);
        
        int fd;
        char * grepargv[] = {GREP, "-n", "-i", "-e", "qu[oia]", NULL};
        
        if( (fd = open(RES_TXT, O_CREAT | O_TRUNC | O_WRONLY, 0640)) < 0 ){
            fprintf(stderr, "Error opening file %s from Child: %s \n", RES_TXT, strerror(errno));
            exit(1);
        }

        dup2(pipefd[READ_END], STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);

        execve(GREP, grepargv, NULL);
        exit(1); //Only if exec fails
    }

    exit(1);
}