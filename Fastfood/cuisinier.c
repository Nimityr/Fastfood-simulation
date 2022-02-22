#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include "fastfood.h"

int main(int argc, char const *argv[]){
	if(argc < 2){
		fprintf(stderr,"Usage : %s <nb_ordre>\n",argv[0]);
		kill(getppid(), SIGUSR1);
	}

	/* Initialisation des variables */

	key_t k;
	carte_spec *carte;
	int i, nb_spec, nb_catust, ctl, no=atoi(argv[1]), fid, semid, shmid, nb_ust, file_affichage_etat;
	commande_cuis comm_cuis;
	struct sembuf *ust_requis_p, *ust_requis_v;
	commande_cuis_prete comm_cuis_p;
	changement_etat etat_cuisinier;

	/* Récupération des IPC */

	k=ftok(FICHIER_LISTE_SERV_CUIS, 1);
	if(k < 0){
		perror("ftok ");
		kill(getppid(), SIGUSR1);
	}

	/*Récupération de la file entre les serveurs et les cuisiniers*/
	fid = msgget(k, 0);
	if(fid < 0){
		if(errno == EEXIST)
			fprintf(stderr, "Erreur : file (cle=%d) existante\n", k);
		else
			perror("Erreur lors de la creation de la file");
		kill(getppid(), SIGUSR1);
	}

	k=ftok(FICHIER_SEMAPHORES, 2);
	if(k < 0){
		perror("ftok ");
		kill(getppid(), SIGUSR1);
	}

	/*Récupération du sémaphore représentant les ustensiles*/
	semid = semget(k, 0, 0);
	if(semid < 0){
		perror("semget: ");
		exit(-1);
	}

	k=ftok(FICHIER_DONNEES, 2);
	if(k < 0){
		perror("ftok ");
		kill(getppid(), SIGUSR1);
	}

	/*Récupération de la carte*/
	shmid = shmget(k, 0, 0);
	if(shmid < 0){
		perror("shmget ");
		exit(-1);
	}

	carte = (carte_spec*)shmat(shmid,NULL,0);
	if(carte == (void*)-1){
		perror("shmat ");
		exit(-1);
	}

	k = ftok(FICHIER_AFFICHAGE, 1);
	if(k < 0){
		perror("ftok ");
		kill(getppid(), SIGUSR1);
	}
	if((file_affichage_etat=msgget(k, 0))==-1) {
		perror("Erreur à la création d'une file entre les serveurs et les clients !\n");
		kill(getppid(), SIGINT);
	}



	/*Initalisation de variables*/
	nb_spec = carte->nb_spec;
	nb_catust = carte->nb_categories_ust;

	ust_requis_p = (struct sembuf*)malloc(sizeof(struct sembuf)*nb_catust);
	ust_requis_v = (struct sembuf*)malloc(sizeof(struct sembuf)*nb_catust);

	for(i = 0; i<nb_catust; i++) {
		ust_requis_p[i].sem_flg = 0;
		ust_requis_v[i].sem_flg = 0;
	}

	srand(getpid());

	etat_cuisinier.type = 3;
	etat_cuisinier.no = no;

	while(1){
		//Envoie à l'affichage des états : le cuisinier attend
		etat_cuisinier.etat = 0;
		if(msgsnd(file_affichage_etat, &etat_cuisinier, sizeof(changement_etat)-sizeof(long), 0) == -1){
			perror("Erreur envoie message à affiche état");
			kill(getppid(), SIGUSR1);
		}


		/* Le cuisinier reçoit le travail à faire */

		if(msgrcv(fid, &comm_cuis, sizeof(commande_cuis)-sizeof(long), 1, 0) == -1){
			perror("Cuisinier: Erreur lors de la reception de la commande");
			kill(getppid(), SIGUSR1);
		}


		/* Le cuisinier consulte la carte pour déterminer le nombre d'ustensiles de chaque catégorie nécessaire à la réalisation de la commande */
		/*nb_ust représente le nombre de catégories d'ustensiles que le serveur à besoin pour la spécialité demandé*/
		nb_ust = 0;
		for(i = 0; i<nb_catust; i++) {
			/*Si le nombre d'ustensiles d'une catégorie i est supérieur à 0 pour la spécialité demandé, on stocke la catégorie d'usyensile qu'il faut utiliser et combien il en faut*/
			if(carte->spec[comm_cuis.commande][i]>0) {
				ust_requis_p[i].sem_num = i;
				ust_requis_v[i].sem_num=  i;
				ust_requis_p[i].sem_op= - carte->spec[comm_cuis.commande][i];
				ust_requis_v[i].sem_op=  carte->spec[comm_cuis.commande][i];
				nb_ust+=1;
			}
			
		}

		//Envoie à l'affichage des états : le cuisinier attend que les ustensiles soient libres
		etat_cuisinier.etat = 1;
		if(msgsnd(file_affichage_etat, &etat_cuisinier, sizeof(changement_etat)-sizeof(long), 0) == -1){
			perror("Erreur envoie message à affiche état");
			kill(getppid(), SIGUSR1);
		}

		/*Opération p pour prendre les ustensiles nécessaires*/
		if(semop(semid, ust_requis_p, nb_ust)==-1) {
			perror("Cuisinier: Erreur de l'opération p pour les ustensiles");
			kill(getppid(), SIGUSR1);
		}
		//Envoie à l'affichage des états : le cuisinier prépare la commande
		etat_cuisinier.etat = 2;
		if(msgsnd(file_affichage_etat, &etat_cuisinier, sizeof(changement_etat)-sizeof(long), 0) == -1){
			perror("Erreur envoie message à affiche état");
			kill(getppid(), SIGUSR1);
		}
		/*Préparation commande*/		
		sleep(2+(rand()%3));


		/*Opération v pour rendre les ustensiles*/
		if(semop(semid, ust_requis_v, nb_ust)==-1) {
			perror("Cuisinier: Erreur de l'opération p pour les ustensiles");
			kill(getppid(), SIGUSR1);
		}
		//Envoie à l'affichage des états : le client à fini de préparer la commande et la donne au serveur
		etat_cuisinier.etat = 3;
		if(msgsnd(file_affichage_etat, &etat_cuisinier, sizeof(changement_etat)-sizeof(long), 0) == -1){
			perror("Erreur envoie message à affiche état");
			kill(getppid(), SIGUSR1);
		}


		/* Le cuisinier notifie au serveur que la commande est prête */
		comm_cuis_p.type = (long)comm_cuis.serveur;
		comm_cuis_p.commande = comm_cuis.commande;
		if(msgsnd(fid, &comm_cuis_p, sizeof(commande_cuis_prete)-sizeof(long), 0) == -1){
			perror("Cuisinier: Erreur lors de l'envoi de la commande");
			kill(getppid(), SIGUSR1);
		}

	}

	return 0;
}