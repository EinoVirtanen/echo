#ifndef PORTLIST_H_
#define PORTLIST_H_

struct portnode {
  int port;
  struct portnode *next;
};

void addUsedPort(struct portnode *root, int addedPort);
void removeUsedPort(struct portnode *root, int removedPort);
int portUsed(struct portnode *root, int port);
void printUsedPorts(struct portnode *root);

#endif
