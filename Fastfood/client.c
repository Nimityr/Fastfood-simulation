#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/msg.h>
#include "fastfood.h"

int main(int argc, char const *argv[]) {
	if(argc < 2){
		fprintf(stderr,"Usage : %s <nb_specialite>\n",argv[0]);
		kill(getppid(), SIGUSR1);
	}
	key_t cle;
	int nb_specialite = atoi(argv[1]), specialite, i, min = 0, serveur = 0, file_serv, sid_cli, file_affichage_etat;
	pid_t pid = getpid();
	int shmid_serv_cli, sid_files_cli_serv;
	struct sembuf p={0, -1, 0}, v={0, 1, 0};
	client_serv *point_cli_serv;
	client_attente attente;
	commande com;
	reception fin_attente;
	commande_prete com_prete;
	paiement argent_donne;
	reception_commande rec_com;
	changement_etat client_etat;

	srand(pid);


	//Sélection aléatoire d'une spécialité
	specialite = rand()%nb_specialite;


	//------------------------------------------------------------------------
	//-----------------Récupération des IPC requis ---------------------------
	//------------------------------------------------------------------------

	//----On récupère le segment de mémoire contenant les files d'attentes des serveurs

	//Création clé
	if((cle=ftok(FICHIER_DONNEES, 1))==-1) {
		printf("Client %d : Erreur création clé pour l'objet de type client_serv !\n", pid);
		kill(getppid(), SIGUSR1);
	}
	//On récupère le segment
	shmid_serv_cli=shmget(cle, 0, 0);

	if(shmid_serv_cli==-1) {
		printf("Client %d : Erreur recupération du segment de mémoire partagé des files d'attentes des clients !\n", pid);
		kill(getppid(), SIGUSR1);
	}

	//On attache le segment de mémoire au processus
	point_cli_serv = (client_serv*)shmat(shmid_serv_cli,NULL,0);
	if(point_cli_serv == (void*)-1) {
		printf("Client %d : Erreur shmat du segment de mémoire partagé des files d'attentes des clients !\n", pid);
		kill(getppid(), SIGUSR1);
	}



	//---- On récupère les sémaphores binaires des files d'attentes des serveurs

	//Création clé pour les sémaphores binaires des files d'attentes des serveurs
	if((cle=ftok(FICHIER_LISTE_CLIENT_SERV, 1))==-1) {
		printf("Client %d : Erreur clé pour les sémaphores binaires des files d'attentes des serveurs!\n", pid);
		kill(getppid(), SIGUSR1);
	}
	//Récupération des sémaphores binaires des files d'attentes des serveurs
	if((sid_files_cli_serv=semget(cle, 0, 0))==-1) {
		printf("Client %d : Erreur à la récupération des sémaphores binaires des files d'attentes des serveurs !\n", pid);
		kill(getppid(), SIGUSR1);
	}


	//---On récupère le sémaphore limitant le nombre de client possibles dans le fastfood à un instant t
	//Création clé
	if((cle=ftok(FICHIER_SEMAPHORES, 3))==-1) {
		printf("Erreur clé pour le sémaphores représentant le nombre de terminaux\n");
		kill(getppid(), SIGINT);
	}
	//Récupération sémaphore
	sid_cli = semget(cle, 0, 0);
	if(sid_cli==-1) {
		printf("Erreur de création du sémaphore représentant les terminaux !\n");
		kill(getppid(), SIGINT);
	}


	cle = ftok(FICHIER_AFFICHAGE, 1);
	if(cle==-1) {
		printf("Erreur clé pour la file entre les serveurs et les clients !\n");
		kill(getppid(), SIGINT);
	}	
	if((file_affichage_etat=msgget(cle, 0))==-1) {
		printf("Erreur à la création d'une file entre les serveurs et les clients !\n");
		kill(getppid(), SIGINT);
	}

	//------------------------------------------------------------------------
	//------------------------------------------------------------------------
	//------------------------------------------------------------------------	


	//printf("Le client %d rentre dans le fastfood\n", pid);

	//-----------------On cherche le serveur qui a le moins de clients qui attendent-----


	//----Initialisation de la valeur minimale du nombre de clients qui attendent

	//Opération p du sémaphore binaire du serveur 0
	if(semop(sid_files_cli_serv, &p, 1)) {
		printf("Client %d : Erreur semop de l'opération p pour l'initialisation de min !\n", pid);
		kill(getppid(), SIGUSR1);
	}
	//Récupération du nombre de clients qui attendent pour le serveur i
	min = point_cli_serv->nbClientFile[0];

	//Opération v du sémaphore binaire du serveur 0
	if(semop(sid_files_cli_serv, &v, 1)) {
		printf("Client %d : Erreur semop de l'opération v pour l'initialisation de min !\n", pid);
		kill(getppid(), SIGUSR1);
	}


	//----Boucle qui parcours toutes les files d'attentent restantes pour trouver le serveur qui a le moins de clients
	for(i = 1; i<point_cli_serv->nbFiles; i++) {
		p.sem_num = (unsigned short)i;
		v.sem_num = (unsigned short)i;
		//Opération p du sémaphore binaire du serveur i
		if(semop(sid_files_cli_serv, &p, 1)) {
			printf("Client %d : Erreur semop de l'opération p de la boucle pour déterminer la file la plus courte !\n", pid);
			kill(getppid(), SIGUSR1);
		}
		//Si un serveur a moins de clients que tous serveurs précédents, ce serveur devient le minimum
		if(point_cli_serv->nbClientFile[i] < min) {
			min = point_cli_serv->nbClientFile[i];
			serveur = i;
		}
		//Opération v du sémaphore binaire du serveur i
		if(semop(sid_files_cli_serv, &v, 1)) {
			printf("Client %d : Erreur semop de l'opération v de la boucle pour déterminer la file la plus courte !\n", pid);
			kill(getppid(), SIGUSR1);
		}
	}

	//----On ajoute un client à la file d'attente la plus courte trouvée

	p.sem_num = (unsigned short)serveur;
	v.sem_num = (unsigned short)serveur;

	//Opération p du sémaphore binaire du serveur ayant le moins de clients qui attendent
	if(semop(sid_files_cli_serv, &p, 1)) {
		printf("Client %d : Erreur semop de l'opération p pour augmenter de 1 une file d'attente !\n", pid);
		kill(getppid(), SIGUSR1);
	}

	client_etat.type = 1;
	client_etat.no = 0;
	client_etat.etat = 1;
	//Envoie à l'affichage des états : le client recherche la file d'attente la plus courte
	if(msgsnd(file_affichage_etat, &client_etat, sizeof(changement_etat)-sizeof(long), 0) == -1){
		perror("Erreur envoie message à affiche état");
		kill(getppid(), SIGUSR1);
	}

	//On rajoute 1 à la file d'attente
	point_cli_serv->nbClientFile[serveur] += 1;
	//Opération v du sémaphore binaire du serveur ayant le moins de clients qui attendent
	if(semop(sid_files_cli_serv, &v, 1)) {
		printf("Client %d : Erreur semop de l'opération v semop pour augmenter de 1 une file d'attente !\n", pid);
		kill(getppid(), SIGUSR1);
	}



	//----On récupère la file de message du serveur ayant la file d'attente la plus courte

	//Création clé pour la file du serveur
	if((cle = ftok(FICHIER_LISTE_CLIENT_SERV, serveur))==-1) {
		printf("Client %d : Erreur clé pour la file du serveur !\n", pid);
		kill(getppid(), SIGUSR1);
	}	
	//Création file
	if((file_serv=msgget(cle, 0))==-1) {
		printf("Client %d : Erreur à la récupération de la file du serveur !\n", pid);
		kill(getppid(), SIGUSR1);
	}

	//-------------------------------------------------------------------------------


	//----On attend que le serveur soit libre
	attente.type = 1;
	attente.client = pid;
	//Le client envoie un message au serveur pour lui indiquer qu'il attend dans sa file
	if(msgsnd(file_serv, (void*)&attente, sizeof(client_attente) - sizeof(long), 0)==-1) {
		printf("Client %d : Erreur de l'envoie du message d'attente !\n", pid);
		kill(getppid(), SIGUSR1);
	}

	//Envoie à l'affichage des états : client en attente d'un serveur
	client_etat.etat = 2;
	if(msgsnd(file_affichage_etat, &client_etat, sizeof(changement_etat)-sizeof(long), 0) == -1){
		perror("Erreur envoie message à affiche état");
		kill(getppid(), SIGUSR1);
	}

	//Le client attent son tour pour être servit par le serveur
	if(msgrcv(file_serv, (void*)&fin_attente, 0, pid, 0)==-1) {
		printf("Client %d : Erreur de reception du message qui met fin à l'attente du client !\n", pid);
		kill(getppid(), SIGUSR1);
	}


	//---- Le client commande la spécialité choisie	
	com.type = 2;
	com.spec = specialite;
	com.client = pid;
	//Le client envoie sa commande
	if(msgsnd(file_serv, (void*)&com, sizeof(commande) - sizeof(long), 0)==-1) {
		printf("Client %d : Erreur de l'envoie du message d'attente !\n", pid);
		kill(getppid(), SIGUSR1);
	}
	
	//Envoie à l'affichage des états : le client commande et patiente
	client_etat.etat = 3;
	if(msgsnd(file_affichage_etat, &client_etat, sizeof(changement_etat)-sizeof(long), 0) == -1){
		perror("Erreur envoie message à affiche état");
		kill(getppid(), SIGUSR1);
	}
	//Le client est indiqué le prix de la commande pour qu'il paye à un terminal de paiement
	if(msgrcv(file_serv, (void*)&com_prete, sizeof(commande_prete) - sizeof(long), pid, 0)==-1) {
		printf("Client %d : Erreur de reception du message qui met fin à l'attente du client !\n", pid);
		kill(getppid(), SIGUSR1);
	}
	
	
	argent_donne.type = 3;
	argent_donne.prix = com_prete.prix;
	//Le client donne un pourboir ecompris entre 0 et 3 euros
	argent_donne.pourboire=rand()%3;

	//Le client donne de l'argent
	if(msgsnd(file_serv, (void*)&argent_donne, sizeof(paiement) - sizeof(long), 0)==-1) {
		printf("Client %d : Erreur de l'envoie du message d'attente !\n", pid);
		kill(getppid(), SIGUSR1);
	}
	//Envoie à l'affichage des états : le client paye
	client_etat.etat = 4;
	if(msgsnd(file_affichage_etat, &client_etat, sizeof(changement_etat)-sizeof(long), 0) == -1){
		perror("Erreur envoie message à affiche état");
		kill(getppid(), SIGUSR1);
	}

	//Le client reçoit sa commande
	if(msgrcv(file_serv, (void*)&rec_com, sizeof(reception_commande) - sizeof(long), pid, 0)==-1) {
		printf("Client %d : Erreur de reception du message qui met fin à l'attente du client !\n", pid);
		kill(getppid(), SIGUSR1);
	}

	//Envoie à l'affichage des états : le client récupère sa commande
	client_etat.etat = 5;
	if(msgsnd(file_affichage_etat, &client_etat, sizeof(changement_etat)-sizeof(long), 0) == -1){
		perror("Erreur envoie message à affiche état");
		kill(getppid(), SIGUSR1);
	}

	//----Le client quitte la file d'attente du serveur
	//Opération p du sémaphore binaire pour la file d'attente du serveur
	if(semop(sid_files_cli_serv, &p, 1)) {
		printf("Client %d : Erreur semop de l'opération p pour augmenter de 1 une file d'attente !\n", pid);
		kill(getppid(), SIGUSR1);
	}
	//On rajoute 1 à la file d'attente
	point_cli_serv->nbClientFile[serveur] -= 1;
	//Opération v du sémaphore binaire pour la file d'attente du serveur
	if(semop(sid_files_cli_serv, &v, 1)) {
		printf("Client %d : Erreur semop de l'opération v semop pour augmenter de 1 une file d'attente !\n", pid);
		kill(getppid(), SIGUSR1);
	}

	//---On incrémente de 1 la valeur du nombre de clients possibles dans le fastfood
	v.sem_num = 0;
	if(semop(sid_cli, &v, 1)) {
		printf("Client %d : Erreur semop de l'opération v semop pour augmenter de 1 une file d'attente !\n", pid);
		kill(getppid(), SIGUSR1);
	}

	//Envoie à l'affichage des états : le client quitte la fastfood
	client_etat.etat = 6;
	if(msgsnd(file_affichage_etat, &client_etat, sizeof(changement_etat)-sizeof(long), 0) == -1){
		perror("Erreur envoie message à affiche état");
		kill(getppid(), SIGUSR1);
	}

	exit(0);
}