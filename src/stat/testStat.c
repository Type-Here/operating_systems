/*
 ============================================================================
 Name        : testStat.c
 Author      : Man
 Version     :
 Copyright   : GPL 3.0
 Description : Test Stat
 ============================================================================
 * Scrivere Programma C:
 * Crea un file, scrive 10 interi in file,
 * Sposta l'offset di 10 000 000 di byte in avanti e scrive altri 10 interi,
 *
 * A questo punto accede ai metadati del file creato e stampa tutti i valori
 *
 * Stat tiene conto sia della Size che del Numero Dei Blocchi usati nel Disco
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>

#include <time.h>

#define FILE "file.bin"

int main(int argc, char * argv[]) {

	char * file = FILE;

	if(argc < 2){
		fprintf(stderr, "Usage %s 'path/to/file.bin' \n\n", argv[0]);
		printf("Ora per semplicità senza argomenti uso %s \n", FILE);
	} else {
		file = argv[1];
	}

	int fd;
	struct stat sb;
	srandom(time(NULL));

	printf("Inizio Programma \n\n");

	if( (fd = open(file, O_CREAT | O_TRUNC | O_WRONLY, 0640)) < 0 ){
		fprintf(stderr, "Errore Ap File: %s \n", strerror(errno));
		exit(1);
	}

	for(int i = 0; i < 10; i++){
		int num = random();
		if(write(fd, &num, sizeof(int)) < 0){
			fprintf(stderr, "Errore Scrittura 1 File: %s \n", strerror(errno));
			exit(1);
		}
	}

	if( (lseek(fd, 10000000L, SEEK_CUR)) < 0){
		fprintf(stderr, "Errore lseek: %s \n", strerror(errno));
		exit(1);
	}

	for(int i = 0; i < 10; i++){
		int num = random();
		if(write(fd, &num, sizeof(int)) < 0){
			fprintf(stderr, "Errore Scrittura 2 File: %s \n", strerror(errno));
			exit(1);
		}
	}

	fstat(fd, &sb);
	close(fd);

	printf("File in Device: 0x%0lx \tRDev=0x%lx \n", sb.st_dev, sb.st_rdev);
	printf("Size File: %ld \nBlocchi: %ld\nBlocchi in KB = %ld\n", sb.st_size, sb.st_blocks, sb.st_blocks / 2);

	printf("\n -- Uscita -- \n\n");

	return EXIT_SUCCESS;
}
