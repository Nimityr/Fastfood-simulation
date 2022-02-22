#### Le thème
Le but est de simuler un fast-food. il y a 
- des serveurs;
- des clients;
- des cuisiniers.

Les clients entrent dans le fast-food, choississent la file d'attente la plus courte. 
Ils donnent leur commande au serveur, attendent d'être servi, payent et partent.

Les serveurs reçoivent les commandes des clients, les donnent aux cuisiniers, attendent le retour des cuisiniers,
donnent leur commande au clients, puis le font payer via un terminal de paiement. Il y a une quantité donnée
de terminaux de paiement qu'il faut se partager.

Les cuisiniers reçoivent les commandes, les fabriquent, et donnent le résultat au (bon) serveur. Pour la réalisation
d'une commande, ils disposent d'un certain nombre d'ustensibles, en quantité limitée, qu'ils doivent se partager.

L'application est composée d'un processus `main` qui va se charger de :

- créer les tous les IPC nécessaires (file, segment, sémaphore);
- lancer les fils (serveurs,clients,cuisiniers);
- terminer proprement l'ensemble à la reception du signal `SIGINT`.

##### Travail à réaliser 
- Le travail est à réaliser en binôme.
- La date de rendu : le 12 février. 
- votre projet comprendra obligatoirement 4 exécutables : `main`, `client`, `serveur`,`cuisinier`, et 
 un `makefile`.

##### Le programme principal : `main`

C'est lui qui lance et arrête proprement la simulation du fast-food.

```
./main nb_serveurs nb_cuisiniers nb_term nb_spec nb_1 nb_2 ... nb_k
```
- `nb_serveurs` et `nb_cuisiniers` représentent le nombre de serveurs et de cuisiniers à lancer;
- `nb_term` est le nombre de terminaux de paiement disponibles, inférieur au nombre de serveurs;
- `nb_spec` est le nombre de spécialité sur la carte du fast-food;
- `nb_1, nb_2,...,nb_k` représente le nombre d'ustensiles de catégorie 1,2,...,k.

1. `main` crée et initialise les IPC nécessaires à l'application;
2. il doit créer une carte dans un segment partagé. Pour chaque spécialité, il doit préciser 
    le nombre d'ustensiles nécessaires à sa fabrication (la somme soit être non nulle);
3. il lance les serveurs et les cuisiniers;
4. il crée les clients indéfiniment. Choississez vous-même le nombre de clients maximum dans
   le fast-food;
5. la reception du signal SIGINT doit terminer proprement l'application : fin des processus serveurs,
	clients, cuisiniers, et suppression des IPC.

##### Les cuisiniers : `cuisinier`
Un cuisinier, lancé par `main`, reçoit 1 paramètre : son numéro d'ordre. Il se met au travail :

- reception du travail à faire par relevage d'un message dans une file de message;
-  consultation de la carte, pour déterminer le nombre d'ustensiles de chaque categorie nécessaire à la réalisation de
  la commande;
- réservation des ustensiles;
- exécution (sleep ...) de la commande;
- remise à disposition des ustensiles;
- notification au serveur que la commande est prête par envoi d'un message dans la file.

##### Les serveurs : `serveur`
Un serveur, lancé par `main`, reçoit 1 paramètre : son numéro d'ordre. Il se met au travail :

- attente d'une commande d'un client (par le moyen que vous voulez);
- envoi aux cuisiniers (postage dans la file de message);
- reception commande;
- paiement ;
- livraison au client.

##### Les clients : `client`
Chaque client, lancé par le processus `main`, reçoit en paramètre le nombre de spécialités de la carte. Puis, après récupération des IPC qu'il utilise, il doit
- choisir une spécialité;
- se mettre en attente du serveur le moins occupé;
- commander en communiquant au serveur le numéro de la spécialité choisie;
- payer sa commande.

##### La carte
Il s'agit d'un segemnt mémoire partagé, que vous organiserez à votre convenance. 

Elle devra contenir : 

- le nombre de spécialité, le nombre de catégories d'ustensiles;
- pour chaque spécialité, le nombre d'ustensiles de chaque catégories nécessaire à sa réalisation.
  (tirage aléatoire).

À titre d'exemple :
```bash
./main 3 3 2 10 2 2 2 3 2 3 
specialite(0) : ust(0)=1 ust(1)=1 ust(2)=0 ust(3)=3 ust(4)=2 ust(5)=3 
specialite(1) : ust(0)=1 ust(1)=0 ust(2)=0 ust(3)=1 ust(4)=2 ust(5)=3 
specialite(2) : ust(0)=2 ust(1)=1 ust(2)=2 ust(3)=2 ust(4)=0 ust(5)=2 
specialite(3) : ust(0)=1 ust(1)=1 ust(2)=2 ust(3)=0 ust(4)=0 ust(5)=1 
specialite(4) : ust(0)=2 ust(1)=2 ust(2)=2 ust(3)=3 ust(4)=1 ust(5)=3 
specialite(5) : ust(0)=2 ust(1)=0 ust(2)=0 ust(3)=2 ust(4)=2 ust(5)=3 
specialite(6) : ust(0)=1 ust(1)=1 ust(2)=1 ust(3)=2 ust(4)=0 ust(5)=1 
specialite(7) : ust(0)=0 ust(1)=2 ust(2)=2 ust(3)=1 ust(4)=2 ust(5)=0 
specialite(8) : ust(0)=2 ust(1)=0 ust(2)=2 ust(3)=2 ust(4)=1 ust(5)=0 
specialite(9) : ust(0)=2 ust(1)=0 ust(2)=2 ust(3)=2 ust(4)=1 ust(5)=1
```

##### Conseils et notation
- Code lisible et modulaire.
- La réalisation du main uniquement vous assure 7/20.
- Il vaut mieux un sous-ensemble qui marche, qu'un tout bricolé qui n'est pas fonctionnel !
- Prévoir une trace d'exécution appropriée qui permettent de suivre l'évolution du fast-food. 

