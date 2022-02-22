#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include "fastfood.h"


int shmid_serv_cli, shmid_carte, nb_serveurs, 
file_serv_cuis, sid_term, *files_cli_serv, sid_ust, 
sid_cli,sid_files_cli_serv, file_affichage_etat;

void arret(void);

int set_signal_handler(int signo, void (*handler)(int)) {
    struct sigaction sa;
    sa.sa_handler = handler;    // call `handler` on signal
    sigemptyset(&sa.sa_mask);   // don't block other signals in handler
    sa.sa_flags = SA_RESTART;            //  restart system calls
    return sigaction(signo, &sa, NULL);
}
//gère les signaux SIGINT et SIGUSR1
void sig_handler(int signo){
	switch(signo){
		case SIGINT :
			//On attend que tous les fils du main meurts avant de fermer les ipc
			while(wait(NULL)>=0);
			arret();
			break;
		case SIGUSR1 :
			//On tue tous les fils et on ferme les ipc
			kill(0, SIGINT);
	}
}

//Termine proprement les IPC avant d'arrêter le programme
void arret(void) {
	int i;
	printf("Fermeture du fast food ... \n");

	//Fermeture des files clients/serveurs
	for(i = 0; i<nb_serveurs; i++) {
		if(msgctl(files_cli_serv[i], IPC_RMID, 0)==-1) {
			printf("Erreur fermeture file client/serveur");
		}
	}
	free(files_cli_serv);

	//Fermeture des sémaphores binaires des files d'attentes des serveurs
	if(semctl(sid_files_cli_serv, IPC_RMID, 0)==-1) {
		printf("Erreur fermeture du sémaphore pour une file client/serveur");
	}


	//Fermeture du segement de mémoire qui stocke le nombre de clients qui attendent des serveurs
	if(shmctl(shmid_serv_cli,IPC_RMID,0)==-1) {
		printf("erreur fermeture du segement de mémoire des files d'attentes");
	}
	//Fermeture du segement de mémoire qui stocke la carte
	if(shmctl(shmid_carte,IPC_RMID,0)==-1) {
		printf("erreur fermeture du segement de mémoire des files d'attentes");
	}
	//Fermeture des files entre les serveurs et les cuisiniers
	if(msgctl(file_serv_cuis, IPC_RMID, 0)==-1) {
		printf("Erreur fermeture de la file serveurs/cuisiniers\n");
	}
	//Fermeture du sémaphore des terminaux de paiement
	if(semctl(sid_term,0,IPC_RMID)==-1) {
		printf("Erreur fermeture du sémaphore représentant les terminaux de paiement\n");
	}
	//Fermeture des sémaphores représentant le nombre d'ustensile
	if(semctl(sid_ust,0,IPC_RMID)==-1) {
		printf("Erreur fermeture du sémaphore représentant les terminaux de paiement\n");
	}

	//Fermeture du sémaphore représentant le nombre max de clients
	if(semctl(sid_cli,0,IPC_RMID)==-1) {
		printf("Erreur fermeture du sémaphore représentant les terminaux de paiement\n");
	}

	//Fermeture de la file de messages pour affichageEtat.c
	if(msgctl(file_affichage_etat, IPC_RMID, 0)==-1) {
		printf("Erreur fermeture file client/serveur");
	}

	exit(0);		
}

int main(int argc, char const *argv[]) {
	key_t cle;
	client_serv cli_serv, *point_cli_serv;
	int i, j, nb_cuisiniers, nb_terminaux, nb_spec, *nb_ust, nb_categories_ust, nb_ust_total, nb_ust_spec;
	carte_spec carte, *point_carte;
	unsigned short *conversion_nb_ust;
	pid_t pid;
	char buf[sizeof(int)], buf2[sizeof(int)];
	struct sembuf p={0, -1, 0};
	unsigned short *val_init;
	changement_etat client_supp;

	srand(getpid());

	//-----------------Vérification des paramètres-------------------

	//On vérifie le nombre d'arguments
	if(argc<6) {
		printf("Nombre d'argument incorrect : %s nb_serveurs nb_cuisiniers nb_term nb_spec nb_1 ... nbk\n", argv[0]);
		exit(2);
	}

	//On vérifie que le nombre de serveurs ne dépasse pas MAX_SERV
	nb_serveurs = atoi(argv[1]);
	if(nb_serveurs>MAX_SERV) {
		printf("Trop de serveurs ont été demandés ! Le maximum est %d et vous avez demandé %d serveurs\n", MAX_SERV, nb_serveurs);
		exit(2);
	}

	
	//On vérifie que le nombre de cuisiniers ne dépasse pas MAX_CUIS
	nb_cuisiniers= atoi(argv[2]);
	if(nb_cuisiniers>MAX_CUIS) {
		printf("Trop de cuisiniers ont été demandés ! Le maximum est %d et vous avez demandé %d cuisiniers\n", MAX_CUIS, nb_cuisiniers);
		exit(2);
	}

	//On vérifie que le nombre de terminaux est inférieur au nombre de serveurs
	nb_terminaux = atoi(argv[3]);
	if(nb_terminaux>=nb_serveurs) {
		printf("Il faut moins de terminaux que de serveurs !\nVous avez demandé %d terminaux et %d serveurs\n", nb_terminaux, nb_serveurs);
		exit(2);
	}

	//nombre de spécialité
	nb_spec = atoi(argv[4]);
	//nb_categories_ust représente le nombre de catégories d'ustensiles
	nb_categories_ust = argc - 5;

	//On vérifie que le nombre de spécialités ne dépasse pas MAX_SPEC
	if(nb_spec>MAX_SPEC) {
		printf("Trop de spécialités ont été demandés ! Le maximum est %d et vous avez demandé %d spécialités\n", MAX_SPEC, nb_spec);
		exit(2);
	}

	//On vérifie que le nombre de catégores d'ustensile ne dépasse pas MAX_UST
	if(nb_categories_ust>MAX_UST) {
		printf("Trop de catégories d'ustensiles ont été demandés ! Le maximum est %d et vous avez demandé %d catégories d'ustensiles\n", MAX_UST, nb_categories_ust);
		exit(2);
	}

	//nb_ust représente le nombre d'ustensile par catégorie donné en argumant sur la ligne de commande
	nb_ust = (int *)malloc(sizeof(int) * nb_categories_ust);
	for(i = 0; i<nb_categories_ust; i++) {
		if((nb_ust[i] = atoi(argv[i+5]))<=0) {
			printf("Une des catégories d'ustensiles a un nombre d'ustensile qui ne covient pas !\
	La catégorie %d a %d ustensiles\n", i, nb_ust[i]);
		}
	}
	//------------------------------------------------------------------------------------------



	//Modification du comportement de SIGINT et pour fermer correctement tous les IPC.
	if(set_signal_handler(SIGINT,sig_handler)==-1 || set_signal_handler(SIGUSR1,sig_handler)==-1) {
		printf("Erreur set_signal_handler !\n");
		exit(2);
	}



	//-----------------Création des files entre les clients et les serveurs-------------------

	//Initialisation de l'espace qui stocke les id des files pour pouvoir fermer les files
	files_cli_serv = (int*)malloc(nb_serveurs * sizeof(int));
	//Initialisation de la variable contenant le nombre de files
	cli_serv.nbFiles = nb_serveurs;

	//initialisation d'un tableau de unsigned short des sémaphores binaires
	val_init = (unsigned short*)malloc(sizeof(unsigned short)*nb_serveurs);

	/// Création des files pour clients/serveurs
	for(i = 0; i<nb_serveurs; i++) {
		//Création clé pour la file
		cle = ftok(FICHIER_LISTE_CLIENT_SERV, i);
		if(cle==-1) {
			printf("Erreur clé pour la file entre les serveurs et les clients !\n");
			kill(getpid(), SIGINT);
		}	
		//Création file
		if((files_cli_serv[i]=msgget(cle, 0600 | IPC_CREAT))==-1) {
			printf("Erreur à la création d'une file entre les serveurs et les clients !\n");
			kill(getpid(), SIGINT);
		}
		//Mise à 0 du nombre de clients qui attendent le serveur 
		cli_serv.nbClientFile[i] = 0;
		//Valeur initiale du sémaphore binaire de la file d'attente du serveur i
		val_init[i] = 1;

	}

	//Création des sémaphores binaires des files d'attentes des serveurs
	//Création clé
	if((cle = ftok(FICHIER_LISTE_CLIENT_SERV, 1))==-1) {
		printf("Erreur clé pour les sémaphores binaires des files d'attentes des serveurs !\n");
		kill(getpid(), SIGINT);
	}
	//Création des sémaphores binaires des files d'attentes des serveurs
	if((sid_files_cli_serv=semget(cle, nb_serveurs, 0666|IPC_CREAT))==-1) {
		printf("Erreur à la création des sémaphores binaires des files d'attentes des serveurs !\n");
		kill(getpid(), SIGINT);
	}
	//initialisation des sémaphores binaires des files d'attentes des serveurs
	if(semctl(sid_files_cli_serv, 0, SETALL, val_init)==-1) {
		printf("Erreur à l'initialisation dees valeurs des sémaphores binaires des files d'attentes des serveurs\n");
	}
	//On libère val_init puisqu'on en a plus besoin
	free(val_init);


	//Creation d'un segment de mémoire partagé pour accéder aux nombres de clients qui attendent les serveurs

	//Création clé
	cle = ftok(FICHIER_DONNEES, 1);
	if(cle==-1) {
		printf("Erreur clé du segment de mémoire partagé des files d'attentes des clients !\n");
		kill(getpid(), SIGINT);
	}	
	//Création segement de mémoire
	shmid_serv_cli=shmget(cle, sizeof(client_serv), IPC_CREAT|0600);

	if(shmid_serv_cli==-1) {
		printf("Erreur création du segment de mémoire partagé des files d'attentes des clients !\n");
		kill(getpid(), SIGINT);
	}	
	//On attache le segment de mémoire au processus
	point_cli_serv = (client_serv*)shmat(shmid_serv_cli,NULL,0);
	if(point_cli_serv == (void*)-1) {
		printf("Erreur shmat du segment de mémoire partagé des files d'attentes des clients !\n");
		kill(getpid(), SIGINT);
	}

	//On ititialise les valeurs stockées dans le segment de mémoire
	*point_cli_serv = cli_serv;
	//On détache le segment du processus
	if(shmdt((void*)point_cli_serv)==-1) {
		printf("Erreur shmdt du segment de mémoire partagé des files d'attentes des clients !\n");
		kill(getpid(), SIGINT);
	}


	//---------------------------------------------------------------------------




	//-----------------Création de la file entre les serveurs et les cuisiniers---------------------
	//Création clé
	if((cle=ftok(FICHIER_LISTE_SERV_CUIS, 1))==-1) {
		printf("Erreur clé pour la file entre les cuisiniers et les serveurs\n");
		kill(getpid(), SIGINT);
	}

	//Création file
	if((file_serv_cuis = msgget(cle, 0600 | IPC_CREAT))==-1) {
		printf("Erreur création file entre les cuisiniers et les serveurs\n");
		kill(getpid(), SIGINT);
	}
	//---------------------------------------------------------------------------------



	//-----------------Création d'un sémaphore qui représente le nombre de terminaux----------------------
	//Création clé
	if((cle=ftok(FICHIER_SEMAPHORES, 1))==-1) {
		printf("Erreur clé pour le sémaphores représentant le nombre de terminaux\n");
		kill(getpid(), SIGINT);
	}

	//Création sémaphore
	sid_term = semget(cle, 1, 0600 | IPC_CREAT);
	if(sid_term==-1) {
		printf("Erreur de création du sémaphore représentant les terminaux !\n");
		kill(getpid(), SIGINT);
	}
	//Initialisation de la valeur du sémaphore
	if(semctl(sid_term, 0, SETVAL, nb_terminaux)==-1) {
		printf("Erreur d'initialisation du sémaphore représentant les terminaux !\n");
		kill(getpid(), SIGINT);
	}

	//--------------------------------------------------------------------------------


	//-----------------Création de la carte-----------------------------------------

	//initialisation du nombre de spécialités
	carte.nb_spec = nb_spec;
	//initialisation du nombre de catégories d'ustensiles 
	carte.nb_categories_ust = nb_categories_ust;

	//On génère aléatoirement le nombre d'ustensiles pour une spécialité i
	for(i = 0; i<nb_spec; i++) {
		for (j = 0; j < nb_categories_ust; j++) {
			//Génération d'un nombre aléatoire d'ustensiles en fonction du nombre maximale d'ustensiles de la catégorie j
			nb_ust_total += carte.spec[i][j] = rand()%nb_ust[j];
		}
		/*Si le nombre d'ustensile totale pour une spécialité = 0, 
		* on modifie la valeur d'une catégorie d'ustensile choisit aléatoirement à 1
		*/
		if(nb_ust_total==0) {
			carte.spec[i][rand()%(nb_categories_ust-1)]=1;
		}else {
			nb_ust_total = 0;
		}
		//On print les ustensiles nécessaire pour une spécialité i
		printf("specialite(%d) : ",i);
		for(j = 0; j<nb_categories_ust; j++) {
			printf("ust(%d)=%d ", j, carte.spec[i][j]);
		}
		printf("\n");
	}

	//Creation d'un segment de mémoire partagé pour la carte

	//Création clé
	cle = ftok(FICHIER_DONNEES, 2);
	if(cle==-1) {
			printf("Erreur clé pour le segment de mémoire partagé de la file entre les serveurs et les clients !\n");
			kill(getpid(), SIGINT);
	}	
	//Création segement de mémoire
	shmid_carte=shmget(cle, sizeof(carte_spec), IPC_CREAT|0600);

	if(shmid_carte==-1) {
		printf("Erreur segment de mémoire partagé de la file entre les serveurs et les clients !\n");
		kill(getpid(), SIGINT);
	}	
	//On attache le segment de mémoire au processus
	point_carte = (carte_spec*)shmat(shmid_carte,NULL,0);
	if(point_carte == (void*)-1) {
		printf("Erreur shmat pour la file entre les serveurs et les clients !\n");
		kill(getpid(), SIGINT);
	}

	//On ititialise les valeurs stockées dans le segment de mémoire
	*point_carte= carte;
	//On détache le segment du processus
	if(shmdt((void*)point_carte)==-1) {
		printf("Erreur shmdt pour la file entre les serveurs et les clients\n");
		kill(getpid(), SIGINT);
	}

	//-------------------------------------------------------------------------------


	//-----------------Création de sémaphores représentant les ustensiles-----------
	//Création clé pour les sémaphores
	if((cle=ftok(FICHIER_SEMAPHORES, 2))==-1) {
		printf("Erreur clé pour le sémaphore représentant les ustensiles\n");
		kill(getpid(), SIGINT);
	}
	//Création sémaphores
	if((sid_ust=semget(cle, nb_categories_ust, 0600|IPC_CREAT))==-1) {
		printf("Erreur de création des sémaphores représentant les ustensiles !\n");
		kill(getpid(), SIGINT);
	}
	//On convertit les valeurs de nb_ust en unsigned short pour initialiser les valeur des sémaphores
	conversion_nb_ust = (unsigned short*)malloc(sizeof(unsigned short) * nb_categories_ust);
	for(i = 0; i<nb_categories_ust; i++) {
		conversion_nb_ust[i] = (unsigned short)nb_ust[i];
	}
	//Initialisation des valeur des sémaphores
	if(semctl(sid_ust, 0, SETALL, conversion_nb_ust)==-1) {
		printf("Erreur d'initialisation des sémaphores représentant les ustensiles !\n");
		kill(getpid(), SIGINT);
	}
	//On libère nb_ust et conversion_nb_ust puisqu'on en a plus besoin
	free(nb_ust);
	free(conversion_nb_ust);

	//-------------------------------------------------------------------------------


	//-----------------Création de l'affichage des etats du fatsfood----------------

	//----Création de la file pour l'affichage des états
	//Création clé
	cle = ftok(FICHIER_AFFICHAGE, 1);
	if(cle==-1) {
		printf("Erreur clé pour la file entre les serveurs et les clients !\n");
		kill(getpid(), SIGINT);
	}	
	//Création file
	if((file_affichage_etat=msgget(cle, 0600 | IPC_CREAT))==-1) {
		printf("Erreur à la création d'une file entre les serveurs et les clients !\n");
		kill(getpid(), SIGINT);
	}

	//----On lance l'affichage des états
	if((pid = fork())==-1) {
		printf("Erreur fork pour un serveur !\n");
		kill(getpid(), SIGINT);
	}
	if(pid==0) {
		snprintf(buf,sizeof(buf),"%d",nb_serveurs);
		snprintf(buf2,sizeof(buf2),"%d",nb_cuisiniers);
		execl("./affichageEtat","./affichageEtat",buf,buf, NULL);
		printf("Erreur execl pour affichageEtat\n");
		kill(getpid(), SIGINT);
	}	

	//-------------------------------------------------------------------------------


	//-----------------Création des serveurs et cuisiniers---------------

	//Creation des serveurs
	for(i = 0; i<nb_serveurs; i++) {
		if((pid = fork())==-1) {
			printf("Erreur fork pour un serveur !\n");
			kill(getpid(), SIGINT);
		}
		if(pid==0) {
			snprintf(buf,sizeof(buf),"%d",i);
			execl("./serveur","./serveur",buf,NULL);
			printf("Erreur execl pour un serveur\n");
			kill(getpid(), SIGINT);
		}
	}

	//Creation des cuisiniers
	for(i = 0; i<nb_cuisiniers; i++) {
		if((pid = fork())==-1) {
			printf("Erreur fork pour un serveur !\n");
			kill(getpid(), SIGINT);
		}
		if(pid==0) {
			snprintf(buf,sizeof(buf),"%d",i);
			execl("./cuisinier","./cuisinier",buf,NULL);
			printf("Erreur execl pour un cuisinier\n");
			kill(getpid(), SIGINT);
		}
	}

	//-------------------------------------------------------------------------------



	//-----------------Création des clients---------------------------

	//Creation d'un semaphore pour limiter le nombre de clients présent à un moment donné dans le fastfood
	//Création clé
	if((cle=ftok(FICHIER_SEMAPHORES, 3))==-1) {
		printf("Erreur clé pour le sémaphores représentant le nombre de terminaux\n");
		kill(getpid(), SIGINT);
	}

	//Création sémaphore
	sid_cli = semget(cle, 1, 0600 | IPC_CREAT);
	if(sid_term==-1) {
		printf("Erreur de création du sémaphore représentant les terminaux !\n");
		kill(getpid(), SIGINT);
	}
	//Initialisation de la valeur du sémaphore avec le nombre max de clients
	if(semctl(sid_cli, 0, SETVAL, MAX_CLI)==-1) {
		printf("Erreur d'initialisation du sémaphore représentant les terminaux !\n");
		kill(getpid(), SIGINT);
	}

	client_supp.type = 1;
	client_supp.etat = 0;
	client_supp.no = 0;

	//Boucle créant des clients à l'infini à des intervalles aléatoires
	while(1) {
		//Si le nombre maximale de clients n'a pas été atteint, on crée un nouveau client
		if(semop(sid_cli, &p, 1)==-1) {
			printf("Erreur semop pour l'opération p !\n");
			kill(getpid(), SIGINT);
		}
		sleep(rand()%5);
		//Envoie à l'affichage des états : un nouveau client est arrivé dans le fastfood
		if(msgsnd(file_affichage_etat, &client_supp, sizeof(changement_etat)-sizeof(long), 0) == -1){
			perror("Erreur envoie message à affiche état pour rajouter un client");
			kill(getpid(), SIGUSR1);
		}
		if((pid = fork())==-1) {
			printf("Erreur fork pour un client !\n");
			kill(getpid(), SIGINT);
		}
		if(pid==0) {
			snprintf(buf,sizeof(buf),"%d",nb_spec);
			execl("./client","./client",buf,NULL);
			printf("Erreur execl pour un client\n");
			kill(getpid(), SIGINT);
		}
	}



	return 0;
}