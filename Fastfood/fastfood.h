#ifndef FASTFOOD_H
#define FASTFOOD_H

//nombre max de serveurs
#define MAX_SERV 100
//nombre max de cuisiniers
#define MAX_CUIS 100 
//nombre max de spécialités
#define MAX_SPEC 100
//nombre max de de catégories d'ustensiles
#define MAX_UST 95
//nombre max de clients
#define MAX_CLI 50

//Permet de stocker le nombre de client qui attende pour un serveur en particulier
typedef struct {
	int nbFiles;
	int nbClientFile[MAX_SERV];
}
client_serv;

//Représente la carte des spécialités
typedef struct {
	int nb_spec;
	int nb_categories_ust;
	int spec[MAX_SPEC][MAX_UST];
}carte_spec;

//Structure utiliser pour communiquer au serveur que le client attend
typedef struct {
	long type;
	pid_t client;
}client_attente;

//Structure utiliser pour annoncer au client qu'on va le servir
typedef struct{
	long type;
}reception;

//Structure utiliser pour stocker la commande d'un client
typedef struct {
	long type; 
	int spec;
	pid_t client;
}commande;

//Structure utiliser pour indiquer au client combien il doit payer pour sa commande
typedef struct {
	long type;
	int prix;
}commande_prete;

//Structure utiliser pour représenter le paiement d'un client
typedef struct {
	long type;
	int prix;
	int pourboire;
}paiement;

//Structure utiliser pour donner une commande prête au client
typedef struct {
	long type;
	int commande;
}reception_commande;

//Structure utiliser pour indiquer aux cuisiniers qu'une commande est arrivée
typedef struct {
	long type;
	int commande;
	int serveur;
}commande_cuis;

//Structure utiliser pour indiquer au serveur qu'une commande est prête
typedef struct {
	long type;
	int commande;
}commande_cuis_prete;

//Structure pour déterminier les changement d'état 
typedef struct {
	long type;
	int no;
	int etat;
	
}changement_etat;

//Fichiers utilisés pour ftok
#define FICHIER_LISTE_CLIENT_SERV "liste_client_serv.txt"
#define FICHIER_DONNEES "donnees.txt"
#define FICHIER_LISTE_SERV_CUIS "liste_serv_cuis.txt"
#define FICHIER_SEMAPHORES "semaphores.txt"
#define FICHIER_SEMAPHORES_FILES_CLIENT_SERV "semaphores_files.txt"
#define FICHIER_AFFICHAGE "affichage.txt"

#endif /* FASTFOOD_H */