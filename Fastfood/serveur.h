#ifndef _SERVEUR_H
#define _SERVEUR_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int set_signal_handler(int signo, void (*handler)(int));

typedef struct {
	long type;
	pid_t expediteur;
	int numSpec;
} requete_t;

typedef struct {
	long type;
	int montant;
} reponse_t;

int fid, semid, sop;

#endif