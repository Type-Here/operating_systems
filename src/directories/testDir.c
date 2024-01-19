/*
 ============================================================================
 Name        : testDir.c
 Author      : Man
 Version     : 0.1
 Copyright   : GPL 3.0
 Description : opendir, scandir usage
 ============================================================================
 * TRACCIA 1:
 * Scrivere un programma C che prende da linea di comando un cammino (PATH)
 * di una directory esistente e stampa in output
 * tutte le direntry presenti nella directory
 *
 * TRACCIA 2:
 * Scrivere un programma C che prende su linea di comando una path
 * e stampa tutti le direntries contenute nella directory e sottodirectory
 *
 * =============== ERRORI COMUNI IN RICORSIONE ========================
 * NB: Dimenticare di concatenare i percorsi!
 * NB: Dimenticare di escludere '..' e '.' -> Loop INFINITO!
 * ====================================================================
 *
 * TRACCIA 3:
 * Scrivere un programma C che prende su linea di comando una path e
 * restituisce il cammino più lungo visitando tutte le sottodirectory di PATH
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

//Include for MAXPATHLEN == 4096
#include <sys/param.h>

#include <sys/types.h>
#include <errno.h>

/* -- New Include for Directories Management (Library functions ; man(3) ) -- */
#include <dirent.h>

#define EXERCISE 3 //Only to launch the correct exercise

/*FN DEF */
void traccia1(char * dir);
void traccia2(char * dir);
void traccia3(char * dir, int livello, int * res);


int main(int argc, char * argv[]) {

	int maxLevel2 = -1;

	if(argc < 2){
		fprintf(stderr, "Usage %s 'path/to/dir' \n", argv[0]);
	}

	printf(" === Start === \n");
	printf("-- Traccia %d --\n", EXERCISE);
	/* -- Launch a different exercise -- */

	switch(EXERCISE){
	case 1:
		traccia1(argv[1]);
		break;
	case 2:
		traccia2(argv[1]);
		break;
	default:
		traccia3(argv[1], 0, &maxLevel2);
		printf("\n Livello Massimo: %d \n\n", maxLevel2);
	}

	printf("\n === End ===\n\n");

	return EXIT_SUCCESS;
}
/**
 * Print in Output all direntries in argv[1] directory
 */
void traccia1(char *dir) {

	DIR *dirp;
	struct dirent *direntry;

	if ((dirp = opendir(dir)) == NULL) {
		fprintf(stderr, "Errore Opendir su %s : %s \n", dir, strerror(errno));
		return;
	}
	printf("Traccia 1: Siamo in dir: %s \n", dir);
	while ((direntry = readdir(dirp)) != NULL) {
		printf("%s -> %s \n", direntry->d_name,
				direntry->d_type == DT_DIR ? "dir" : "file");
	}
}

/**
 * Print all direntries of argv[1] dir and subdirectories
 * COMMON ERRORS IN RECURSION:
 *
 * DO NOT forget about path string concatenation
 * DO NOT forget to exclude '..' and '.' -> INFINITE Loop!
 */
void traccia2(char *dir) {
	DIR *dirp;
	struct dirent *direntry;
	char buff[1024];

	//Open directory
	if ((dirp = opendir(dir)) == NULL) {
		fprintf(stderr, "Errore Opendir su %s : %s \n", dir, strerror(errno));
		return;
	}

	printf("\n-Siamo in dir: %s \n", dir);

		//Run through current direntries
	while ((direntry = readdir(dirp)) != NULL) {
		//Print Current Directory content: Name -> Dir or File
		printf("%s -> %s \n", direntry->d_name, direntry->d_type == DT_DIR ? "dir" : "file");

		//Recursion if direntry is dir
		if(direntry->d_type == DT_DIR){
			//Jump '..' and '.' to avoid loop
			if( strcmp(direntry->d_name, ".") == 0 || strcmp(direntry->d_name, "..") == 0 ){
				continue;
			}

			//Set correct path (append to buff)
			sprintf(buff, "%s/%s", dir, direntry->d_name);
			//Recursive Call
			traccia2(buff);
		}
	}

	closedir(dirp);
}

/**
 * Set *res to the longest visited path number of subdirecories of PATH (tree height from 0)
 *
 */
void traccia3(char *dir, int livello, int *res) {

	DIR *dirp;
	struct dirent *direntry;
	char buff[MAXPATHLEN];

	//Set longest path -> tree height
	if (livello > *res) *res = livello;

	//Open directory
	if ((dirp = opendir(dir)) == NULL) {
		fprintf(stderr, "Errore Opendir su %s : %s \n", dir, strerror(errno));
		return;
	}

	//Run through current direntries
	while ((direntry = readdir(dirp)) != NULL) {
		//Recursion if direntry is dir
		if (direntry->d_type == DT_DIR) {
			//Jump '..' and '.' to avoid loop
			if (strcmp(direntry->d_name, ".") == 0 || strcmp(direntry->d_name, "..") == 0) {
				continue;
			}
			//Set correct path (append to buff)
			sprintf(buff, "%s/%s", dir, direntry->d_name);
			//Recursive Call
			traccia3(buff, livello + 1, res);
		}
	}

	closedir(dirp);
}
