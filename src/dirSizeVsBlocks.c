/**
 * Scrivere un programma C che sulla riga di comando legge la path di una directory esistente 
 * e visitando tutto il sottoalbero con radice path 
 * lista i file che contengono uno spazio non allocato stampando la path completa, 
 * la size ed il numero di blocchi.*/

/* Positive Control should work in ext4 fs, not all */

#define _GNU_SOURCE //Needed in Windows Environment while compiling in WSL 
//This implementation of readdir and DT_DIR, DT_REG ecc macros are not POSIX but GNU Specific

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/param.h> //For MAXPATHLEN and MAXNAMELEN (The latter not used here, named for reference)
#include <sys/types.h>
#include <sys/stat.h> //For Info About a Single File 

#include <dirent.h> // Focus Here: Header to manage directories (Library not Syscall!)

#include <errno.h>

struct node{
    char path[MAXPATHLEN];
    struct node * next;
};

int isEmpty(struct node * head){
    return head == NULL;
}

struct node * addNode(struct node * head, char * path){
    struct node * new = malloc(sizeof(struct node));
        if(!new) {fprintf(stderr, "Error Malloc \n"); return head;}
    
    strcpy(new->path, path);
    new->next = head;
    return new;
}


void searchDir(char * path, struct node ** head);

int main(int argc, char * argv[]){
    if(argc < 2){
        fprintf(stderr, "Usage %s <path> \n", argv[0]);
        exit(1);
    }
    struct node * head = NULL;
    
    printf("--- Welcome ---\n");

    searchDir(argv[1], &head);

    if(isEmpty(head)) printf("No Files Found\n");
    else{
        struct node  * temp = head;
        int count = 0;
        while(temp != NULL){
            count++;
            printf("Name: %s \n", temp->path);
            temp = temp->next;
        }

        printf("\nReport: %d Files Found \n", count);
    }

    struct node * toBeFreed = head;
    while(toBeFreed != NULL){
        struct node * elim = toBeFreed;
        toBeFreed = toBeFreed->next;

        free(elim);
;    }
    printf("\n--- End ---\n");
    
    return 0;
}

void searchDir(char * path, struct node ** head){
    struct stat sb;
    struct dirent * entry;
    DIR * dirp;

    char completePath[MAXPATHLEN];

    if( (dirp = opendir(path)) == NULL ){
        fprintf(stderr, "Error Opening %s : %s \n", path, strerror(errno));
        return;
    }

    while((entry = readdir(dirp)) != NULL){
        sprintf(completePath, "%s/%s", path, entry->d_name);
        off_t fixedSize = 0;
        if(entry->d_type == DT_REG){

            if( stat(completePath, &sb) < 0 ){
               fprintf(stderr, "Error Stat On %s : %s \n", completePath, strerror(errno));
               continue; 
            }

            /*This Calculates the size in bytes of logical blocks expected by file size */
            fixedSize = sb.st_size  + (sb.st_blksize - sb.st_size % sb.st_blksize);
            /*Alternative Method*/
            //while(fixedSize < sb.st_size) fixedSize += sb.st_blksize;

            /* Physical Blocks are usually 512 bytes.
            * st_blksize refers to physical blocksand NOT logical! 
            * This mandates an adjustment (fixedSize / 512 should be == to st_blocks) */
            if(fixedSize/512 != sb.st_blocks){

                printf("FixedSize %ld - ", fixedSize/512);
                printf("File: %s - Size: %ld - Total BlockSize: %ld - NumBlocks %ld - BlkSize %ld\n", 
                            entry->d_name, sb.st_size, sb.st_blocks/2, sb.st_blocks, sb.st_blksize);
                *head = addNode(*head, completePath);
            }
        }
        
        else if(entry->d_type == DT_DIR &&
            strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 ){
            searchDir(completePath, head);
        }
    }

    closedir(dirp);
}