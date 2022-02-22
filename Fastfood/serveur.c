#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/msg.h>
#include <errno.h>
#include "fastfood.h"

int main(int argc, char const *argv[]) {
	if(argc < 2){
		fprintf(stderr,"Usage : %s <nb_ordre>\n",argv[0]);
		kill(getppid(), SIGUSR1);
	}

	/* Initialisation des variables */

	key_t k;
	client_attente attente;
	reception recep;
	commande com;
	commande_prete com_p;
	paiement argent;
	reception_commande rec_com;
	commande_cuis comm_cuis;
	commande_cuis_prete comm_cuis_p;
	int no = atoi(argv[1]), fid, sid_term, fid_cuis, file_affichage_etat;
	struct sembuf p={0, -1, 0}, v= {0, 1, 0};
	changement_etat serveur_etat;

	/* Récupération des IPC */

	/*Récupération de la file entre le serveurs et les clients*/
	k=ftok(FICHIER_LISTE_CLIENT_SERV,no);
	if(k < 0){
		perror("ftok ");
		kill(getppid(), SIGUSR1);
	}

	fid = msgget(k, 0);
	if(fid < 0){
		if(errno == EEXIST)
			fprintf(stderr, "Erreur : file (cle=%d) existante\n", k);
		else
			perror("Erreur lors de la creation de la file");
		kill(getppid(), SIGUSR1);
	}


	/*Récupération de la file entre les serveurs et les cuisiniers*/
	k=ftok(FICHIER_LISTE_SERV_CUIS, 1);
	if(k < 0){
		perror("ftok ");
		kill(getppid(), SIGUSR1);
	}

	fid_cuis = msgget(k, 0);
	if(fid < 0){
		if(errno == EEXIST)
			fprintf(stderr, "Erreur : file (cle=%d) existante\n", k);
		else
			perror("Erreur lors de la creation de la file");
		kill(getppid(), SIGUSR1);
	}

	/*Récupération du sémaphore représentant les teminaux*/
	k=ftok(FICHIER_SEMAPHORES, 1);
	if(k < 0){
		perror("ftok ");
		kill(getppid(), SIGUSR1);
	}

	sid_term = semget(k, 0, 0);
	if(fid < 0){
		if(errno == EEXIST)
			fprintf(stderr, "Erreur : file (cle=%d) existante\n", k);
		else
			perror("Erreur lors de la creation du sémaphore pour les terminaux");
		kill(getppid(), SIGUSR1);
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

	comm_cuis.type = 1;
	comm_cuis.serveur = getpid();
	
	serveur_etat.type = 2;
	serveur_etat.no = no;
	

	srand(getpid());

	while(1){
		//Envoie à l'affichage des états : le serveur attend qu'un client arrive
		serveur_etat.etat = 0;
		if(msgsnd(file_affichage_etat, &serveur_etat, sizeof(changement_etat)-sizeof(long), 0) == -1){
			perror("Erreur envoie message à affiche état");
			kill(getppid(), SIGUSR1);
		}

		/* Le serveur attend qu'un client arrive */
		if(msgrcv(fid, &attente, sizeof(client_attente)-sizeof(long), 1, 0) == -1){
			perror("Serveur: Erreur lors de la reception de la commande");
			kill(getppid(), SIGUSR1);
		}

		recep.type = (long)attente.client;
		/*Le serveur annonce au client qu'il peut commander*/
		if(msgsnd(fid, &recep, 0, IPC_NOWAIT) == -1){
			perror("Serveur: Erreur lors de l'envoi de la commande");
			kill(getppid(), SIGUSR1);
		}
		//Envoie à l'affichage des états : le serveur attend la commande d'un client
		serveur_etat.etat = 1;
		if(msgsnd(file_affichage_etat, &serveur_etat, sizeof(changement_etat)-sizeof(long), 0) == -1){
			perror("Erreur envoie message à affiche état");
			kill(getppid(), SIGUSR1);
		}
		/*Le serveur attend la commande du client*/
		if(msgrcv(fid, &com, sizeof(commande)-sizeof(long), 2, 0) == -1){
			perror("Serveur: Erreur lors de la reception de la commande");
			kill(getppid(), SIGUSR1);
		}

		//Envoie à l'affichage des états : le serveur récupère la commande
		serveur_etat.etat = 2;
		if(msgsnd(file_affichage_etat, &serveur_etat, sizeof(changement_etat)-sizeof(long), 0) == -1){
			perror("Erreur envoie message à affiche état");
			kill(getppid(), SIGUSR1);
		}
		sleep(1+rand()%3);

		/* Le serveur envoit aux cuisiniers la commande */

		comm_cuis.commande = com.spec;

		if(msgsnd(fid_cuis, &comm_cuis, sizeof(commande_cuis)-sizeof(long), IPC_NOWAIT) == -1){
			perror("Serveur: Erreur lors de l'envoi de la commande");
			kill(getppid(), SIGUSR1);
		}

		//Envoie à l'affichage des états : le serveur attend que la commande soit prête
		serveur_etat.etat = 3;
		if(msgsnd(file_affichage_etat, &serveur_etat, sizeof(changement_etat)-sizeof(long), 0) == -1){
			perror("Erreur envoie message à affiche état");
			kill(getppid(), SIGUSR1);
		}

		/* Le serveur reçoit la commande prête */
		if(msgrcv(fid_cuis, &comm_cuis_p, sizeof(commande_cuis_prete)-sizeof(long), getpid(), 0) == -1){
			perror("Serveur: Erreur lors de la reception de la commande");
			kill(getppid(), SIGUSR1);
		}


		//Envoie à l'affichage des états : le serveur attend qu'un terminal de paiement soit libre
		serveur_etat.etat = 4;
		if(msgsnd(file_affichage_etat, &serveur_etat, sizeof(changement_etat)-sizeof(long), 0) == -1){
			perror("Erreur envoie message à affiche état");
			kill(getppid(), SIGUSR1);
		}
		/* Paiement */
		/* Réserve un terminal de paiement*/
		if(semop(sid_term, &p, 1)==-1) {
			perror("Serveur: Erreur lors de la récupération d'un terminal");
			kill(getppid(), SIGUSR1);
		}

		//Envoie à l'affichage des états : le serveur réserve le terminal
		serveur_etat.etat = 5;
		if(msgsnd(file_affichage_etat, &serveur_etat, sizeof(changement_etat)-sizeof(long), 0) == -1){
			perror("Erreur envoie message à affiche état");
			kill(getppid(), SIGUSR1);
		}

		/*Le serveur demande au client de payé*/
		com_p.type = attente.client;
		com_p.prix = com.spec +10;
		if(msgsnd(fid, &com_p, sizeof(commande_prete)-sizeof(long), IPC_NOWAIT) == -1){
			perror("Serveur: Erreur lors de l'envoi de la commande");
			kill(getppid(), SIGUSR1);
		}

		//Envoie à l'affichage des états : le serveur attend le paiement du client
		serveur_etat.etat = 6;
		if(msgsnd(file_affichage_etat, &serveur_etat, sizeof(changement_etat)-sizeof(long), 0) == -1){
			perror("Erreur envoie message à affiche état");
			kill(getppid(), SIGUSR1);
		}
		/*Le serveur reçoit l'argent et un pourboire*/
		if(msgrcv(fid, &argent, sizeof(paiement)-sizeof(long), 3, 0) == -1){
			perror("Serveur: Erreur lors de la reception de la commande");
			kill(getppid(), SIGUSR1);
		}

		
		/* Livraison au client de la commande*/
		rec_com.type = (long)attente.client;
		rec_com.commande = com.spec;
		if(msgsnd(fid, &rec_com, sizeof(reception_commande)-sizeof(long), IPC_NOWAIT) == -1){
			perror("Serveur: Erreur lors de l'envoi de la commande");
			kill(getppid(), SIGUSR1);
		}

		//Envoie à l'affichage des états : le serveur donne la commande au client
		serveur_etat.etat = 7;
		if(msgsnd(file_affichage_etat, &serveur_etat, sizeof(changement_etat)-sizeof(long), 0) == -1){
			perror("Erreur envoie message à affiche état");
			kill(getppid(), SIGUSR1);
		}

		/*Le serveur quitte le terminal de paiement*/
		if(semop(sid_term, &v, 1)==-1) {
			perror("Serveur: Erreur lors de la récupération d'un terminal");
			kill(getppid(), SIGUSR1);
		}

	}
	return 0;
}