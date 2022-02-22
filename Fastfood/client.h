#ifndef _CLIENT_H
#define _CLIENT_H

#include <stdlib.h>
#include <stdio.h>

void usage_strtol(const char *str);

struct sembuf ops[] = {
	{0, -1, 0},	//vérouille la ressource
	{0, 1, 0}	//déverouille la ressource
};

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