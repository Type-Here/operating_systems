/**
 * Programma C
 * - Prima carica in un array di stringhe tutti i contenuti in una directory 
 *   specificata da linea di comando.
 * - Poi crea 4 figli ognuno dei quali processerà 1/4 dei file nell’array.
 * - Per ogni file verrà eseguito SHA1(filename). 
*/

#define _GNU_SOURCE // richiesta perchè alcune macro e direttive per le dir non
                    // sono POSIX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <unistd.h>

#include <errno.h>
// #include per sha1

#define NCHD 4
#define DEFSIZE 255

void addString(char **arr, int *size, int *count, char *name) {
    if (*count < *size) {
        arr[(*count)++] = strdup(name);
    
    } else {
        char ** arr2 = realloc(arr, sizeof(char *) *(*size + 255));
        *size += 255;
        arr = arr2;
        arr[(*count)++] = strdup(name);
    }
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage %s <path/to/dir> \n", argv[0]);
        exit(1);
    }
    pid_t pid[NCHD];
    DIR *dirp;
    struct dirent *entry;
    int size = DEFSIZE;
    int numPerChild, lastChild;
    int count = 0;
    char **nomi = malloc(size * sizeof(char *));

    if ((dirp = opendir(argv[1])) == NULL) {
        fprintf(stderr, "Error Opening Dir: %s \n", strerror(errno));
        exit(1);
    }

    /* Si poteva usare la scandir direttamente */
    while ((entry = readdir(dirp)) != NULL) {
        addString(nomi, &size, &count, entry->d_name);
    }

    closedir(dirp);

    numPerChild = count / (NCHD);
    lastChild = count - (NCHD - 1) * numPerChild;

    for (int i = 0; i < NCHD; i++) {
        if ((pid[i] = fork()) < 0) { // Error Handling
            fprintf(stderr, "Error Fork num %d : %s \n", i, strerror(errno));
            exit(1);
        }

        else if (pid[i] == 0) { // Child

            int start = numPerChild * i;
            int end;

            if(i == NCHD - 1){
                end = start + lastChild;
                printf("Child %d: %d  %d - Count %d ", i, start, end, count);
                printf("- lastChild %d\n", lastChild);
            } else { 
                end = start + numPerChild;
                printf("Child %d: %d  %d - Count %d ", i, start, end, count);
                printf("- PerChild %d\n", numPerChild);
            }

            
            
            usleep(10000); //Only for better display

            for (int j = start; j < end; j++) {
                printf("Child %d: %s \n", i, nomi[j]);
                //sha1(nomi[i]);
            }
            
            exit(0);
        }
        else { // Parent
        }
    }

    /* Parent */
    for (int i = 0; i < NCHD; i++) {
        waitpid(pid[i], NULL, WUNTRACED);
    }

    for (int i = 0; i < size; i++) {
        free(nomi[i]);
    }

    free(nomi);
    
    return 0;
}