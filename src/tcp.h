#ifndef NWP_TEMPLATE_H
#define NWP_TEMPLATE_H

#include <stdint.h>
#include <stddef.h>
#include <sys/socket.h>

int tcpget(int sockfd, int port);
int tcpsend(int sockfd, int port);

#endif
