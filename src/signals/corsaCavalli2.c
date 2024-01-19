/*
Scrivere un programma C che crea 10 processi figli. 
Ogni processo una volta avviato entra in un loop ed aspetta (pause()) eventuali segnali. 
Ogni volta che riceve il segnale SIGUSR1 viene incrementato il contatore cnt, attende 200 msec e rinvia il segnale SIGUSR1 ad uno dei 10 processi scelto a caso. Quando la variabile
cnt raggiunge il valore 100 il processo termina. Il processo padre attende il primo figlio che termina ed uccide
tutti gli altri
*/
