#ifndef _CUISINIER_H
#define _CUISINIER_H

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

struct sembuf ops[] = {
	{0, -1, 0},	//vérouille la ressource
	{0, 1, 0}	//déverouille la ressource
};

int fid, semid, shmid, sop;
#endif