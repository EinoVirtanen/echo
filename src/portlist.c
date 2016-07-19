#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L // Newest POSIX definition for everything
#endif

#include <stdlib.h>
#include <stdio.h>
#include "portlist.h"

void addUsedPort(struct portnode *root, int addedPort) {
	//HACK - problems with calling remove function after child process dies
	if(addedPort > 59999)
		for(int i = 49153;i < 59999;i++)
			removeUsedPort(root, i);
	//END HACK
	while(root->next != NULL) {
		root = root->next;
	}
	struct portnode *new;
    new = (struct portnode *) malloc(sizeof(struct portnode));
    new->port = addedPort;
    new->next = NULL;
    root->next = new;
}

void removeUsedPort(struct portnode *root, int removedPort) {
	struct portnode *prev;
    prev = (struct portnode *) malloc( sizeof(struct portnode) );
	do {
		if (root->port == removedPort) {
			prev->next = root->next;
			free(root);
			break;
		}
		prev = root;
		root = root->next;
	}while(root->next != NULL);
}

int portUsed(struct portnode *root, int port) {
	do {
		if (root->port == port)
			return 1;
		root = root->next;
	} while(root != NULL);
	return 0;
}

void printUsedPorts(struct portnode *root) {
	printf("Ports in use:\n");
	printf("%d\n", root->port);
	while(root->next != NULL) {
		root = root->next;
		printf("%d\n", root->port);
	}

}

