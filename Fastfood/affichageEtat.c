#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <unistd.h>
#include "fastfood.h"

int main(int argc, char const *argv[]) {
	if(argc < 2){
		fprintf(stderr,"Usage : %s <nb_serveurs> <nb_cuisiniers>\n",argv[0]);
		kill(getppid(), SIGUSR1);
	}
	int i, j, nb_clients[7], nb_serveurs = atoi(argv[1]), nb_cuisiniers = atoi(argv[2]),
	serveurs[nb_serveurs], cuisiniers[nb_cuisiniers], file_affichage_etat, ancien_etat;
	key_t cle;
	changement_etat changement;

	//On récupère la file pour l'affichage des états
	cle = ftok(FICHIER_AFFICHAGE, 1);
	if(cle==-1) {
		printf("Erreur clé pour la file entre les serveurs et les clients !\n");
		kill(getppid(), SIGINT);
	}	
	if((file_affichage_etat=msgget(cle, 0))==-1) {
		printf("Erreur à la création d'une file entre les serveurs et les clients !\n");
		kill(getppid(), SIGINT);
	}

	//On initialise les tableaux 
	for (i = 0; i < nb_serveurs; i++) {
		serveurs[i] = -1;
	}
	for (i = 0; i < nb_cuisiniers; i++) {
		cuisiniers[i] = -1;
	}
	for (i = 0; i <7; i++) {
		nb_clients[i] = 0;
	}

	while(1) {
		//On attend la réception d'un message indication qu'un état a été changé
		if(msgrcv(file_affichage_etat, &changement, sizeof(changement_etat)-sizeof(long), 0, 0) == -1){
			perror("Serveur: Erreur lors de la reception de la commande");
			kill(getppid(), SIGUSR1);
		}
		//Si le message est du type 1, on a un client qui a changé d'état et on affichage les nouveaux états des clients
		if(changement.type==1) {
			if(changement.etat==0) {
				nb_clients[0]+=1;
			}else {
				nb_clients[changement.etat]+=1;
				nb_clients[changement.etat-1]-=1;
			} 
			printf("Nombre de clients qui viennent d'arriver : %d\n", nb_clients[0]);
			printf("Nombre de clients qui cherchent une file d'attente : %d\n", nb_clients[1]);
			printf("Nombre de clients qui attendent dans une file : %d\n", nb_clients[2]);
			printf("Nombre de clients qui commandent: %d\n", nb_clients[3]);
			printf("Nombre de clients qui payent pour leur commande : %d\n", nb_clients[4]);
			printf("Nombre de clients qui reçoivent leur commandes : %d\n", nb_clients[5]);
			printf("Nombre de ventes totales : %d\n\n", nb_clients[6]);
			
		}

		//Si le message est du type 2, on a un serveur qui a changé d'état et on affichage les nouveaux états des serveurs
		if(changement.type==2) {
			//En fonction de l'état du serveur i, on affiche son état
			serveurs[changement.no]=changement.etat;
			printf("serveurs qui attendent un client : " );
			for( i = 0; i<nb_serveurs; i++) {
				if(serveurs[i]==0) {
					printf("%d ", i);
				}
				
			}
			printf("\n");
			printf("serveurs qui attendent la commande d'un client: " );
			for( i = 0; i<nb_serveurs; i++) {
				if(serveurs[i]==1) {
					printf("%d ", i);
				}
				
			}
			printf("\n");
			printf("serveurs qui récupèrent la commande d'un client : " );
			for( i = 0; i<nb_serveurs; i++) {
				if(serveurs[i]==2) {
					printf("%d ", i);
				}
				
			}
			printf("\n");
			printf("serveurs qui attendent la préparation d'une commande : " );
			for( i = 0; i<nb_serveurs; i++) {
				if(serveurs[i]==3) {
					printf("%d ", i);
				}
				
			}
			printf("\n");
			printf("serveurs qui attendent qu'un terminal soit libre : " );
			for( i = 0; i<nb_serveurs; i++) {
				if(serveurs[i]==4) {
					printf("%d ", i);
				}
				
			}
			printf("\n");
			printf("serveurs qui réservent des terminaux de paiement : " );
			for( i = 0; i<nb_serveurs; i++) {
				if(serveurs[i]==5) {
					printf("%d ", i);
				}
				
			}
			printf("\n");
			printf("serveurs qui attendent le paiement de la commande : " );
			for( i = 0; i<nb_serveurs; i++) {
				if(serveurs[i]==6) {
					printf("%d ", i);
				}
				
			}
			printf("\n");
			printf("serveurs qui donnent la commande au client : " );
			for( i = 0; i<nb_serveurs; i++) {
				if(serveurs[i]==7) {
					printf("%d ", i);
				}
				
			}
			printf("\n\n");
		}

		//Si le message est du type 3, on a un cuisinier qui a changé d'état et on affichage les nouveaux états des cuisiniers
		if(changement.type==3) {
			//En fonction de l'état du cuisinier i, on affiche son état
			cuisiniers[changement.no]=changement.etat;
			printf("cuisiniers qui attendent un commande: " );
			for( i = 0; i<nb_cuisiniers; i++) {
				if(cuisiniers[i]==0) {
					printf("%d ", i);
				}
				
			}
			printf("\n");
			printf("cuisiniers qui attendent des ustensiles : " );
			for( i = 0; i<nb_cuisiniers; i++) {
				if(cuisiniers[i]==1) {
					printf("%d ", i);
				}
				
			}
			printf("\n");
			printf("cuisiniers qui préparent une commande : " );
			for( i = 0; i<nb_cuisiniers; i++) {
				if(cuisiniers[i]==2) {
					printf("%d ", i);
				}
				
			}
			printf("\n");
			printf("cuisiniers qut ont fini la préparation d'une commande: " );
			for( i = 0; i<nb_cuisiniers; i++) {
				if(cuisiniers[i]==3) {
					printf("%d ", i);
				}
				
			}
			printf("\n\n");
		}
		

	}

	


	return 0;
}