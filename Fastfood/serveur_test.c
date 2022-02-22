#include <stdlib.h>
#include <stdio.h>

int main(int argc, char const *argv[]) {
	int no = atoi(argv[1]);
	printf("Je suis le serveur %d\n", no);
	while(1);
	exit(0);
}