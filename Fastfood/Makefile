FILES = main \
		serveur \
		client \
		cuisinier \
		affichageEtat

test : all
	./main 5 5 4 5 4 7 3

all : ${FILES}

main : main.c fastfood.h
	gcc -o main main.c 

serveur : serveur.c fastfood.h
	gcc -o serveur serveur.c

client : client.c fastfood.h
	gcc -o client client.c

cuisinier : cuisinier.c fastfood.h
	gcc -o cuisinier cuisinier.c

affichageEtat : affichageEtat.c fastfood.h
	gcc -o affichageEtat affichageEtat.c