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

int udpsendcli(char* port, char* hostname) {

    int servsock, fd, n;
    struct sockaddr_in servaddr;
    char sendbuffer[PAYLOAD_SIZE + sizeof(unsigned long long int)];
    int len=sizeof(struct sockaddr_in);
    char filename[80];
    unsigned long long int seqno = 0, maxull = 18000000000000000000; // 8 bytes
    size_t bytesread = 0;
    FILE *filepointer = NULL;

    printf("Starting UDP sending.\n");
    printf("Time to wait between packets (ms)? (recommended values: 10 wired, 100 mobile)\n");

    int userwait = 0;
    scanf("%d", &userwait);

    printf("How many times a packet is sent? (recommended values: 2 wired, 3 mobile)\n");

    int resends = 0;
    scanf("%d", &resends);

    const struct timespec waittime = { 0, userwait * 1000 }; // 10ms

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

    printf("Name of file to send?\n");
    scanf("%s", filename);

    printf("Sending filename %s to server.\n", filename);
    sendto(servsock, filename, sizeof(filename), 0, (struct sockaddr*) &servaddr, len);


    while (filepointer == NULL) {

        //printf("Name of file to send?\n");
        //scanf("%s", filename);

        if (DEBUG)
            printf("Checking if file %s exists.\n", filename);

        filepointer = fopen(filename, "r");

        if (filepointer != NULL) // fopen return NULL as the file pointer if it fails
            break;

        printf("Error, file named %s does not exist.\n", filename);
    }

    //
    // Starting of the file transfer loop.
    //

    if (DEBUG)
        printf("Waiting for 2 seconds and starting the transmission to port %s.\n",
                port); // What would be a better way to do this?

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

            sendto(servsock, sendbuffer, PAYLOAD_SIZE + sizeof(unsigned long long int), 0, (struct sockaddr *)&servaddr, len);
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
