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

#define LISTENQ 5

#define BUFFERSIZE 100000

#define DEBUG 0 // This can be changed to 0 in normal usage

#define PAYLOAD_SIZE 512


int udpsend(int port) {


    struct sockaddr_in6 udpsendservaddr, cliaddr;
    int fd, udpsendsocket;
    unsigned int len_sock = sizeof(struct sockaddr_in6);
    unsigned long long int receivedseqno = 0, neededseqno = 0, maxull = 18000000000000000000;

    char filepath[80], storagelocation[] = "storage/";
    memset(filepath, '\0', sizeof(filepath));
    strcat(filepath, storagelocation);

    if (DEBUG)
        printf("filepath == %s\n", filepath);

    udpsendsocket = socket(AF_INET6, SOCK_DGRAM, 0);
    if (udpsendsocket == -1){
        perror("socket fail");
        return EXIT_FAILURE;
        }

    udpsendservaddr.sin6_family=AF_INET6;
    udpsendservaddr.sin6_port=htons(port);
    udpsendservaddr.sin6_addr=in6addr_any;

    if(bind(udpsendsocket, (struct sockaddr*) &udpsendservaddr, len_sock) == -1){
                perror("bind");
                return -1;
    }
    char udpsendbuffer[BUFFERSIZE];

    if (DEBUG)
    printf("Waiting for first UDP packet containing filename.\n");

    recvfrom(udpsendsocket, &udpsendbuffer, BUFFERSIZE, 0, (struct sockaddr *)&cliaddr, &len_sock);

    if(DEBUG)
    printf("got: %s\n", udpsendbuffer);

    strcat(filepath, udpsendbuffer);

    memset(udpsendbuffer, '\0', BUFFERSIZE);

    if (DEBUG)
        printf("Got filename, checking if %s exists.\n", filepath);


    FILE *filepointer = fopen(filepath, "r");

    if (filepointer != NULL) {
        printf("Error, file named %s exists. Sending error to client and exiting.\n",
                filepath);
        return 0;
    }

     //
     // Creation of the file
     //

    if (DEBUG)
        printf("Creating file %s and starting to write in it.\n", filepath);

    filepointer = fopen(filepath, "w");

    if (filepointer == NULL)
        printf("Error creating file!\n");

    //
    // Main file transfer loop
    //

    while (1) {

        memset(udpsendbuffer, '\0', BUFFERSIZE);

        recvfrom(udpsendsocket, &udpsendbuffer, BUFFERSIZE, 0, (struct sockaddr *) &cliaddr, &len_sock);

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
