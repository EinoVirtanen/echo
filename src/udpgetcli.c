#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L // Newest POSIX definition for everything
#endif

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
#include <sys/uio.h>
#include <netdb.h>
#include <time.h>

#define LISTENQ 5

#define BUFFERSIZE 1000000

#define DEBUG 0 // This can be changed to 0 in normal usage

#define PAYLOAD_SIZE 512

int udpgetcli(char* port, char* hostname) {

    printf("got port %s\n", port);
    int servsock, fd, n;
    struct sockaddr_in servaddr;
    char sendbuffer[PAYLOAD_SIZE + sizeof(unsigned long long int)];
    int len=sizeof(struct sockaddr_in);
    char filename[80];
    unsigned long long int receivedseqno = 0, neededseqno = 0, maxull = 18000000000000000000;
    size_t bytesread = 0;
    FILE *filepointer = NULL;
    char udpsendbuffer[BUFFERSIZE];


    if (DEBUG)
        printf("Got port %s and ip %s from client.c.\n", port, hostname);


    servsock = socket(AF_INET, SOCK_DGRAM ,0);
    if (servsock == -1){
        perror("socket");
        return -1;
        }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(port));
    if (inet_pton(AF_INET, hostname, &servaddr.sin_addr) == 0){
                printf("inet_pton\n");
                return -1;
        }

    printf("Name of file to download?\n");
    scanf("%s", filename);

    printf("Sending filename %s to server.\n", filename);
    sendto(servsock, filename, sizeof(filename), 0, (struct sockaddr*) &servaddr, len);

    //TODO I had no time to implement correct file check
    printf("Opening file and starting file transfer.\nIt's possible to get stuck here, if file isn't found on server. Use tcp if unsure about file existence\n");    

    filepointer = fopen(filename, "w");

    if (filepointer == NULL)
        printf("Error creating file!\n");

    //
    // Starting of the file transfer loop.
    //
    while (1) {
        memset(udpsendbuffer, '\0', BUFFERSIZE);

        recvfrom(servsock, &udpsendbuffer, BUFFERSIZE, 0, (struct sockaddr *) &servaddr, &len);

        memcpy(&receivedseqno, udpsendbuffer, sizeof(unsigned long long int));

        if (receivedseqno > maxull) {
            size_t lastpacketsize = receivedseqno - maxull;

            if (DEBUG)
                printf("DEBUG: lastpacketsize == %d.\n", lastpacketsize);

            fwrite(udpsendbuffer + sizeof(unsigned long long int), 1,
                    lastpacketsize, filepointer);

            fclose(filepointer);

            printf("File received successfully. Killing process.\n");
            return 0;

        }

        if (receivedseqno == neededseqno) {
            if (DEBUG)
                printf("Expected packet number %llu, got %llu. Writing to file.\n",
                        neededseqno, receivedseqno);
            fwrite(udpsendbuffer + sizeof(unsigned long long int), 1,
            PAYLOAD_SIZE, filepointer);
            neededseqno++;
        } else {
            if (DEBUG)
                printf("Expected packet number %llu, got %llu. Discarding packet.\n",
                        neededseqno, receivedseqno);
        }

    }

}
