#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <dirent.h>
#include <linux/limits.h>

#define LISTENQ 5

#define DEBUG 0 // This can be changed to 0 in normal usage

int udpsend(int port);
