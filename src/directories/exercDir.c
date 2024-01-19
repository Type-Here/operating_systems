/**
 * 1)Stampare gli stat di tutti i file della Working directory e delle sub directory
 * 2)Cercare ricorsivamente il file più grande contenuto in un path
 * 3)Stampare ricorsivamente tutti i file contenuti in una directory e sotto directory appartenenti a device a cui appartiene la directory
*/

#define _GNU_SOURCE //Needed in Windows Environment while compiling in WSL 
//This implementation of readdir and DT_DIR, DT_REG ecc macros are not POSIX but GNU Specific

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/param.h> //For MAXPATHLEN and MAXNAMELEN (The latter not used here, named for reference)
#include <sys/types.h>
#include <sys/stat.h> //For Info About a Single File
//#include <sys/sysmacros.h> 

#include <time.h> //ctime for formatting stat sb-st_atim, sb-st_ctim, sb-st_mtim
#include <dirent.h> // Focus Here: Header to manage directories (Library not Syscall!)

#include <errno.h>
#define ES 3

enum ESERCIZIO {ES1 = 1, ES2 = 2, ES3 = 3};

void returnStat(char * path);
int readStat(char * file);
void checkBiggestFile(char * path, char * name, off_t * size);
int printFilesOfSubDirs(char * path, dev_t devId);
int printfDirFilesSameDevice(char * path);

/**
 * MAIN Lauch Different Exercises
*/
int main(int argc, char * argv[]){
    int ex = ES;

    if(argc < 2){
        fprintf(stderr, "Usage %s <path/to/dir> [numExercise(Optional):1|2|3] \n", argv[0]);
    }

    if(argv[2] != NULL){
        ex = atoi(argv[2]);
    }

    char nameLongestFile[MAXPATHLEN];
    off_t longestSize = -1;
    
    printf(" === Start === \n");

    switch (ex)
    {
    case ES1:
        printf("-- Stat of Working Dir and its subdirs -- \n");

        returnStat(argv[1]);     
        break;

    case ES2:
        printf("-- Biggest File in Path Recursive -- \n");

        checkBiggestFile(argv[1], nameLongestFile, &longestSize);
        printf("Biggest File in %s:\n", argv[1]);
        printf("\tName: %s\n\tSize: %ld Bytes | %ldKB \n", nameLongestFile, longestSize, longestSize / 1024 );
    
        break;

    default:
        printf("Opening %s\n", argv[1]);
        printfDirFilesSameDevice(argv[1]);
        break;
    }
    
    printf("\n === End === \n");

    return 0;
}


/**
 * Ex 1: Return Stat of All Files in a Dir and all SubDir
 * @param path valid starting directory
*/
void returnStat(char * path){
    struct dirent * entry;
    char longPath[MAXPATHLEN];
    DIR * dirp;

    if( (dirp = opendir(path)) == NULL ){
        fprintf(stderr, "Errore Apertura Cartella %s : %s \n", path, strerror(errno));
        return;
    }

    while( (entry = readdir(dirp)) != NULL ){
        printf("%s -> %s \n", entry->d_name, entry->d_type == DT_DIR ? "d" : "f");
        sprintf(longPath, "%s/%s", path, entry->d_name);
        
        if(entry->d_type == DT_REG){
            readStat(longPath);
        
        } else if(entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 ){
            returnStat(longPath);
        }
    }

    closedir(dirp);
}

/**
 * Used in ReturnStat, Effectively read the stat from a file and print info
 * @param file complete path to file to use in stat
 * @return 0 if success, -1 if error
*/
int readStat(char * file){
    struct stat sb;

    if( (stat(file, &sb) ) < 0 ){
        fprintf(stderr, "Errore Stat su %s : %s \n", file, strerror(errno));
        return -1;
    }
    printf("\tSize: %ld\n", sb.st_size);
    printf("\tMode: %o\n", sb.st_mode);
    printf("\tBlocchi: %ld -BlkSize: %ld \n", sb.st_blocks, sb.st_blksize);
    printf("\tInode: %ld\n", sb.st_ino);
    printf("\tDevice: %ld\n", sb.st_dev);
    printf("\tLink Count: %ld\n", sb.st_nlink);
    printf("\tOwnership: UID:%d - GID:%d \n", sb.st_uid, sb.st_gid);
    printf("\tLast Change:       %s", ctime((time_t *) &sb.st_ctim));
    printf("\tLast Access:       %s", ctime((time_t *) &sb.st_atim));
    printf("\tLast modification: %s", ctime((time_t *) &sb.st_mtim));

    return 0;
}

/**
 * Find Biggest File of a Dir and all SubDir
 * @param path dir to read from
 * @param name pointer to string where fn writes the biggest filename (complete path use MAXPATHLEN to alloc/initiate it)
 * @param size point of a off_t where fn writes  the size of the file in bytes
*/
void checkBiggestFile(char * path, char * name, off_t * size){
    char completePath[4096];
    struct stat sb;

    struct dirent * entry;
    DIR * dirp;
    
    if( (dirp = opendir(path)) == NULL ){
        fprintf(stderr, "Errore Apertura %s: %s \n", path, strerror(errno));
        return;
    }

    while( (entry = readdir(dirp)) != NULL ){
        sprintf(completePath, "%s/%s", path, entry->d_name);
        
        if(entry->d_type == DT_DIR && strcmp(entry->d_name, ".") !=0 && strcmp(entry->d_name, "..") != 0){
            checkBiggestFile(completePath, name, size);

        } else {
            
            if( stat(completePath, &sb) < 0 ){
                fprintf(stderr, "Errore Lettura Stat di %s : %s \n", completePath, strerror(errno));
                return;
            }

            if(sb.st_size > *size){
                strcpy(name, completePath);
                *size = sb.st_size;
            }    
        }
    }

    closedir(dirp);
}

/**
 * Print all files from path (all subdirs) of a specific devId
 * @param path dir to start from
 * @param devId specific devId to read from
*/
int printFilesOfSubDirs(char * path, dev_t devId){
    char completePath[MAXPATHLEN];
    struct stat sb;

    DIR * dirp;
    struct dirent * entry;

    if( (dirp = opendir(path)) == NULL ){
        fprintf(stderr, "Errore Apertura Dir %s : %s \n", path, strerror(errno));
        return -1;
    }
    
    while(  (entry = readdir(dirp)) != NULL ){
        sprintf(completePath, "%s/%s", path, entry->d_name);
        printf(" File %s -> %s\n", entry->d_name, entry->d_type == DT_DIR ? "d" : "f");

        if(entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0){
            
            if(stat(completePath, &sb) < 0 ){
               fprintf(stderr, "Error Stat %s : %s \n", completePath, strerror(errno));
               return -1; 
            }
            if(sb.st_dev == devId){
                printf("Opening %s:\n", entry->d_name);
                printFilesOfSubDirs(completePath, devId);
            } else {
                printf("%s different devId: %ld - Jumped\n", entry->d_name, sb.st_dev);
            }
        }
    }

    closedir(dirp);
    return 0;
}

/**
 * Check the Directory and see its devId then it launch printFilesOfSubDirs with that specific devId
 * @param path dir to start from (its devId will be used)
 * @return 0 is success, -1 otherwise
*/
int printfDirFilesSameDevice(char * path){
    struct stat sb;

    if( (stat(path, &sb)) < 0 ){
        fprintf(stderr, "Error Stat %s : %s \n", path, strerror(errno));
        return -1;
    }
    printf(" DevId of %s: %ld \n", path, sb.st_dev);
    
    if(printFilesOfSubDirs(path, sb.st_dev) < 0){
        fprintf(stderr, "Error in Recursive Function \n");
        return -1;
    };

    return 0;
}