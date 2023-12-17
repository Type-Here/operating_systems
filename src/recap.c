/**
 * Scrivere un Programma C :
 * - Prende come parametri 2 argomenti (path) e crea 3 figli.
 * - I primi due cercano ricorsivamente il file più grande all'interno di ciascuna path poi andranno in pausa.
 * - Il terzo esegue il comando ' find <path/primo/argoment> -maxdepth 1 -type f -size +3M '
 *   e restitusce il risultato al padre.
 * - Il padre controlla ciclicamente se i figli abbiano finito i quali, terminata la ricerca,
 *   inviano i dati del file trovato (nome + metadata).
 * - Il padre poi controlla se il risultati siano tutti e tre uguali fra loro e identifica il file più grande 
 *   (escluso il file dell'ultimo figlio).
 * - Il padre poi manderà un segnale SIGUSR1 al figlio che ha trovato il file più grande (>=) 
 *   e un segnale di terminazione all'altro.
 * - Il figlio che riceve SIGUSR1 stamperà infine a video le principali caratteristiche del file che ha trovato.
 * - Poi le salverà in un file chiamato "result.txt" (tronca se esiste).
 * - Poi andrà a metà del file (+/- offset di char) e aggiungerà la riga "Sono Mercoledì in Mezzo alla Settimana".
 * - Poi terminerà anche lui.
 * - Il padre attenderà la terminazione di tutti i figli.
 * 
 * Usa SysCall dove possibile.
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/param.h>

#include <errno.h>

#define READ_END 0
#define WRITE_END 1

#define FINDPATH "/usr/bin/find"
#define RES_TXT "result.txt"
#define NUMCHILD 3

void firstsChildrenWorker(char * path, int pipefd[2]);
void child3Worker(char * path, int pipefd[2]);
int fatherWorker(pid_t * pid, int (*pipefd)[2]);

void findBigger(char * path, char * result, off_t * size);

char * fileToPrintStat;

struct statAndName{
    char name[MAXPATHLEN];
    struct stat sb;
};

/* === MAIN === */

int main(int argc, char * argv[]){
    if(argc < 3){
        fprintf(stderr, "Usage %s <path/to/dir1> <path/to/dir2> \n", argv[0]);
        exit(1);
    }

    pid_t pid[NUMCHILD];
    int pipefd[NUMCHILD][2];

    printf(" --- Welcome --- \n\n");

    for(int i = 0; i < NUMCHILD; i++){
        /* Create Pipe */
        if( (pipe2(pipefd[i], O_NONBLOCK)) < 0 ){
            fprintf(stderr, "Error Opening Pipe n %d: %s \n", i, strerror(errno));
            exit(1);
        }

        /* Create Children */
        if( (pid[i] = fork()) < 0 ){ //Error Handling
            fprintf(stderr, "Error Creating Child n %d: %s \n", i, strerror(errno));
            exit(1);

        } else if(pid[i] == 0){ // I'm in child
            
            for(int j = 0; j <= i; j++){
               close(pipefd[j][READ_END]);
               if(j != i) close(pipefd[j][WRITE_END]);
            }
            
            (i != NUMCHILD - 1) ? firstsChildrenWorker(argv[i+1], pipefd[i]) : child3Worker(argv[1], pipefd[i]);
            
            exit(1); //JIC

        } else { //I'm in father
           close(pipefd[i][WRITE_END]);
        }
    }

    fatherWorker(pid, pipefd);
    return 0;
}

/* ================ WORKERS AND HANDLERS ================ */

/**
 * Find Biggest REG File in path and save its path and size
 * @param path path to search from
 * @param result pointer to string where save (temporary) result completePath
 * @param size pointer to off_t where save (temporary) result size
*/
void findBigger(char * path, char * result, off_t * size){
    struct stat sb;
    struct dirent * entry;
    DIR * dirp;
    char completePath[MAXPATHLEN];
    
    /*Open Current Dir*/
    if( (dirp = opendir(path)) == NULL ){
        fprintf(stderr, "Error Opening %s: %s \n", path, strerror(errno));
        return;
    }
    
    /*Read Current Dir Entries*/
    while( (entry = readdir(dirp)) != NULL){
        sprintf(completePath, "%s/%s", path, entry->d_name);
        
        /*If regular file control size*/
        if(entry->d_type == DT_REG){
            if( stat(completePath, &sb) < 0 ){
                fprintf(stderr, "Error Using Stat on %s : %s \n", completePath, strerror(errno));
                //exit(1);
                return;
            }

            if(sb.st_size > *size){
                *size = sb.st_size;
                strcpy(result, completePath);
            }

        /*If Dir check recursively*/    
        } else if(entry->d_type == DT_DIR && 
                    strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0){
            findBigger(completePath, result, size);
        }
    }

    closedir(dirp);
}

/* ===== SIG HANDLERS ===== */

/**
 * Handler for SIGUSR1 Signal
*/
void handlerSigUsr(int sig){
    printf("INFO: SIG%s received by %d\n", sigabbrev_np(sig), getpid());
    struct stat sb;
    int fd;
    pid_t mypid = getpid();

    off_t middleSeek;
    ssize_t bytesRead;

    char buffer[10240];
    char temp[5124];
    char * addtxt = {"Sono Mercoledì in Mezzo alla Settimana"};

    printf("\n%d: Stat Result for %s: \n", mypid, fileToPrintStat);
    
    if( stat(fileToPrintStat, &sb) < 0 ){
        fprintf(stderr, "Error Stat from Handler %d: %s \n", mypid, strerror(errno));
        exit(1);   
    }
    
    sprintf(buffer, "\tSize: %ld Bytes\n\tDevId: %ld \n\tBlockSize: %ld \tNumBLock: %ld \n\tLast Time Modified: %s\n", 
                                sb.st_size, sb.st_dev, sb.st_blksize, sb.st_blocks,  ctime((time_t *) &sb.st_mtim));                     
    printf("%s", buffer);

    printf("\n%d: Saving results to file \n", mypid);

    /*Writing Stat Results*/

    if( (fd = open(RES_TXT, O_WRONLY | O_CREAT | O_TRUNC, 0640)) < 0 ){
        fprintf(stderr, "Error Opening File %s: %s \n", RES_TXT, strerror(errno));
        exit(1);  
    }

    if(write(fd, buffer, strlen(buffer)) < 0){
        fprintf(stderr, "Error Writing Handler to %s: %s \n", RES_TXT, strerror(errno));
        exit(1);  
    }

    if( (middleSeek = lseek(fd, 0L, SEEK_CUR)) < 0){
        fprintf(stderr, "Error LSeek in %s: %s \n",RES_TXT, strerror(errno));
        exit(1);  
    }

    close(fd);

    /* Adding Text in The middle ( text after needs to be reappended ) */

    if( (fd = open(RES_TXT, O_RDWR | O_DIRECT)) < 0 ){
        fprintf(stderr, "Error Second Opening File %s: %s \n", RES_TXT, strerror(errno));
        exit(1);  
    }

    if( ( lseek(fd, middleSeek/2 + (middleSeek % 4), SEEK_SET)) < 0){
        fprintf(stderr, "Error Second LSeek in %s: %s \n", RES_TXT, strerror(errno));
        exit(1);  
    }

    if((bytesRead = read(fd, temp, 5124)) < 0){
        fprintf(stderr, "Error Read from %s: %s \n", RES_TXT, strerror(errno));
        exit(1); 
    }

    if( ( lseek(fd, middleSeek/2 + (middleSeek % 4), SEEK_SET)) < 0){
        fprintf(stderr, "Error Second LSeek in %s: %s \n", RES_TXT, strerror(errno));
        exit(1);  
    }

    if(write(fd, addtxt, strlen(addtxt)) < 0){
        fprintf(stderr, "Error Second Writing Handler to %s: %s \n", RES_TXT, strerror(errno));
        exit(1);  
    }

    /* ReAppend Old Text After Middle Insertion */

    if(write(fd, temp, strlen(temp)) < 0){
        fprintf(stderr, "Error Last Writing Handler to %s: %s \n", RES_TXT, strerror(errno));
        exit(1);  
    }

    close(fd);
    printf("%d: File Saved And Modified... Exiting \n", mypid);
    exit(0);
}



/**
 * Handler for SIGINT Signal
*/
void exitHandler(int sig){
    printf("PID: %d - SIG%s received... Exiting\n", getpid(), sigabbrev_np(sig));
    exit(0);
}


/* ============= FIRST AND SECOND CHILD WORKER ============ */

/**
 * First And Second Child Worker Function
 * @param path dir path where find biggest file
 * @param pipefd[2] pipe to communicate with the parent
*/
void firstsChildrenWorker(char * path, int pipefd[2]){
    char result[MAXPATHLEN];
    struct statAndName sb;
    off_t size = 0;

    close(pipefd[READ_END]);
    
    /* Find Largest File in Path */
    findBigger(path, result, &size);
    printf("From Child %d: Found %s\n", getpid(), result);
    
    /* Save info about largest file*/
    strcpy(sb.name, result);
    
    if(stat(result, &sb.sb) < 0){
        fprintf(stderr, "Error Exec Stat in Child %d to get Result File Info: %s \n", getpid(), strerror(errno));
        exit(1);
    }

    /* Send Info to Parent */
    if(write(pipefd[WRITE_END], &sb, sizeof(struct statAndName)) < 0){
        fprintf(stderr, "Error Writing in pipe to Parent the stat info from %d: %s \n", getpid(), strerror(errno));
        exit(1); 
    }

    /* Save Pointer to File Name in Global Variable to be used in handlerSigUsr */
    fileToPrintStat = result;
    
    printf("Child %d: Has Written\n", getpid());

    close(pipefd[WRITE_END]);
    

    /* Getting Signals */
    if(signal(SIGUSR1, handlerSigUsr) == SIG_ERR){
        fprintf(stderr, "Error Setting Signal from %d: %s \n", getpid(), strerror(errno));
        return;
    }

    if(signal(SIGINT, exitHandler) == SIG_ERR){
        fprintf(stderr, "Error Setting Signal from %d: %s \n", getpid(), strerror(errno));
        return;
    }

    /* Pause While Waiting*/
    while(1){
        if(pause() < 0){
            printf("PID: %d Pause interrupted \n", getpid());
        }
    }

    exit(0);
}

/* ============ THIRD CHILD WORKER ============ */

/**
 * Third Child Worker
 * @param path dir path where to launch find
 * @param pipefd[2] pipe to communicate with the parent
*/
void child3Worker(char * path, int pipefd[2]){
    close(pipefd[READ_END]);

    char command[] = {"-type f -size +3M -print -quit"};
    char * rest = command;
    char * token;
    char * argvfind[9]; /* TODO: Set Larger Capacity for Different Command */

    argvfind[0] = FINDPATH;
    argvfind[1] = path;
    int ar = 2;
    
    while( (token = strtok_r(rest, " ", &rest)) ) { argvfind[ar++] = token; }
    argvfind[ar] = NULL;

    printf("Child %d: Will launch find\n", getpid());

    if( dup2(pipefd[WRITE_END], STDOUT_FILENO) < 0){
        fprintf(stderr, "Error Dup2 From Child 3: %s \n", strerror(errno));
        exit(1); 
    }
    close(pipefd[WRITE_END]);

    /*for(int i = 0; i < ar + 1; i++){
        printf("argvfind[%d]: %s \n", i, argvfind[i]);
    }*/

    if( execve(FINDPATH, argvfind, NULL) < 0 ){
        fprintf(stderr, "Error Exec Find From Child 3: %s \n", strerror(errno));
        exit(1); 
    }

    exit(1); // It Should Never Arrive Here
}


/* ================= PARENT WORKER ================== */

/**
 * Parent Worker
 * @param pipefd array of file descriptors for each child
*/
int fatherWorker(pid_t * pid, int (*pipefd)[2]){
    //for(int i = 0; i < NUMCHILD; i++) close(pipefd[i][WRITE_END]);

    int childIndexLive, childIndexTerm; //Used for Signals
    int check = NUMCHILD; //Used in While, == 0 when all children have ended sending data to parent 
    int closedchild[NUMCHILD] = {0};
    ssize_t bytesRead;
   
    char buffer[sizeof(struct statAndName)] = {0};
    
    char result3[MAXPATHLEN] = {0};
    struct statAndName childstat[NUMCHILD - 1];
    
    /* Beacuse MAXPATHLEN is used in read, we receive seoareted data (name first, then stat) (except last child)
     these pointers keep trace of where data is written in structs statAndName */
    void * pointer[NUMCHILD - 1]; 
    for(int i = 0; i < NUMCHILD - 1; i++){
        pointer[i] = &childstat[i];
    }

    /* Read From O_NONBLOCK Pipes */
    while(check > 0){

        for(int i = 0; i < NUMCHILD; i++){
            if(closedchild[i]) continue; //If Child i is set as closed, continue

            if( (bytesRead = read(pipefd[i][READ_END], buffer, MAXPATHLEN)) < 0 ){ //Error Handling
                if(errno != EWOULDBLOCK && errno != EAGAIN){
                    fprintf(stderr, "Error Par Read From Child %d: %s \n", i, strerror(errno));
                    exit(1); 
                } else {
                    //printf("Child %d hasn't done yet.\n", i);
                }

            } else if(bytesRead > 0) { //Receive Data
                if(i != NUMCHILD - 1){
                    memcpy(pointer[i], buffer, bytesRead);
                    pointer[i] += bytesRead;

                } else { //Handling Last Child that exec find
                    strcpy(result3, buffer);
                    result3[strcspn(result3, "\n")] = 0; //remove '\n'
                }

                memset(buffer, 0, sizeof(buffer));

            } else { //bytesRead == 0 (Read Ended)
                closedchild[i] = 1;
                --check;
            }

        }
        usleep(100000); //Wait For Child to Do Things
    }
    
    /* Printing Results From Pipes */
    printf("\nPARENT Results:\n\tChild 1: %s %ld bytes\n\tChild 2: %s %ld bytes\n\tChild 3: %s\n\n", 
                    childstat[0].name, childstat[0].sb.st_size, childstat[1].name, childstat[1].sb.st_size, result3);

    (strcmp(childstat[0].name, childstat[1].name) == 0 && strcmp(childstat[1].name, result3) == 0) ? 
                               printf("PARENT: Files Found Are Equal \n") : printf("PARENT: Files are Different \n");
    
    usleep(100000);

    /*Find Larger File between first and second child. Then set appropriate signals*/

    (childstat[0].sb.st_size >= childstat[1].sb.st_size) ? (childIndexLive = 0) : (childIndexLive = 1);
    childIndexTerm = !childIndexLive; //Shortcut because there are only 2 children

    if(kill(pid[childIndexLive], SIGUSR1) < 0){
        fprintf(stderr, "Error Sending SIGUSR1 to Child 1: %s \n", strerror(errno));
        exit(1);
    }

    if(kill(pid[childIndexTerm], SIGINT) < 0){
        fprintf(stderr, "Error Sending SIGINT to Child 2: %s \n", strerror(errno));
        exit(1);
    }

    for(int i = 0; i < NUMCHILD; i++){
        waitpid(pid[i], NULL, WUNTRACED);
    }

    return 0;
}