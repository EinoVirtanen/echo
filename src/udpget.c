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

#define BUFFERSIZE 100000

#define DEBUG 0 // This can be changed to 0 in normal usage

#define PAYLOAD_SIZE 512


int udpget(int port) {

    printf("got port %d\n", port);
    struct sockaddr_in6 udpsendservaddr, cliaddr;
    int fd, udpsendsocket;
    unsigned int len_sock = sizeof(struct sockaddr_in6);
    unsigned long long int seqno = 0, maxull = 18000000000000000000; // 8 bytes
    size_t bytesread = 0;
    FILE *filepointer = NULL;
    char sendbuffer[PAYLOAD_SIZE + sizeof(unsigned long long int)];

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


    filepointer = fopen(filepath, "r");

    if (filepointer == NULL) {
        printf("Error, file named %s doesn't exist. Sending error to client and exiting.\n",
                filepath);
        return 0;
    }

    //
    // Main file transfer loop
    //


    //TODO run out of time when implementing user asked values for resends and time between packets.
    //This implementation will use 100ms and 3 packets as default.

    int resends = 3;
    int userwait = 100;
    const struct timespec waittime = { 0, userwait * 1000 }; // 10ms
    printf("after timestruct\n");

    //if (DEBUG)
    //    printf("Waiting for 2 seconds and starting the transmission to port %s.\n", port); // What would be a better way to do this?

    printf("Starting transmission.\n");

    sleep(2);

    while (1) {
 
       if (DEBUG)
            printf("Nulling sendbuffer.\n");

        memset(sendbuffer, '\0', sizeof(sendbuffer));

        if (DEBUG)
            printf("Reading a maximum of 512 bytes to sendbuffer.\n");

        bytesread = fread(sendbuffer + sizeof(unsigned long long int), 1,
        PAYLOAD_SIZE, filepointer); // Reading the payload to the memory offset by the size of the sequence number

        if (DEBUG)
            printf("Read %zu bytes to sendbuffer.\nSending payload x3.\n",
                    bytesread);

        if (bytesread < 512) { // If under 512 bytes read, the end of file was reached
            seqno = maxull + bytesread; // Sum the bytes read to the seqno so the server can retrieve the data
        }

        if (DEBUG)
            printf("Appending seqno to sendbuffer.\n");

        memcpy(sendbuffer, &seqno, sizeof(unsigned long long int)); // Copy the sequence number to the sendbuffer

        for (int cntr = 0; cntr < resends; cntr++) { // Send the packets three times to combat possible packet loss

            sendto(udpsendsocket, sendbuffer, PAYLOAD_SIZE + sizeof(unsigned long long int), 0, (struct sockaddr *)&cliaddr, len_sock);
            nanosleep(&waittime, NULL); // Wait for the amount of time given by the user

        }

        if (seqno > maxull) { // Close file when seqno > maxull
            printf("File sent successfully!\n");
            fclose(filepointer);
            return 0;
        }

        seqno++;

    }

}
