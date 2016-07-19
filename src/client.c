#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L // Newest POSIX definition for everything
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include "udpsendcli.h"
#include "udpgetcli.h"

#define DEBUG 0 // This can be changed to 0 in normal usage

int getMenuChoice(void) {
    int menuChoice = -1;
    while (menuChoice < 1 || menuChoice > 5) {
        printf(
                "1: FILELIST\n2: TCPSEND\n3: UDPSEND\n4: TCPGET\n5: UDPGET\nGive command number: ");
        scanf("%d", &menuChoice);
    }
    return menuChoice;
}

int main(int argc, char **argv) {

    printf("Running 'echo' file transfer client.\n");

    if (DEBUG)
        printf("Debug mode on.\n");

    int sockfd, sec_sockfd, n, cmd, count, recvbytes;
    char ipinput[46], usercmd[2], filename[80], recv_port[161], filebuffer[512];
    struct sockaddr_in servaddr, sec_servaddr;

    // Open a stream (TCP) IPv4 socket, and check if successful
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error opening socket!\n");
        return -1;
    }

    printf("Give IP address of the file server: ");
    scanf("%s", ipinput);

    int server_port = 6666;

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, ipinput, &servaddr.sin_addr) <= 0) {
        printf("inet_pton error for %s!\n", argv[1]);
        return -1;
    }

    // Connect to IP address and port indicated by servaddr
    // Check if it was succesful
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("TCP connect error!\n");
        return -1;
    }
    /*printf(
     "1: FILELIST\n2: TCPSEND\n3: UDPSEND\n4: TCPGET\n5: UDPGET\nGive command number: ");
     scanf("%d", &usercmd);*/
    usercmd[0] = getMenuChoice();

    //write command to server
    write(sockfd, usercmd, sizeof(char));
    if (DEBUG)
        printf("wrote cmd %d to server\n", usercmd[0]);

    cmd = (int) usercmd[0];

    switch (cmd) {
    case 1:
        printf("\n==File listing==\n");
        //sleep(1);  //Not sure if these sleeps are necessary.
        while ((n = read(sockfd, recv_port, 160)) > 0) {
            printf("%s", recv_port);
        }
        break;

    case 2:
        printf("\n==TCP SEND==\n");

        read(sockfd, recv_port, 160);
        if (DEBUG)
            printf("Server wants connection to port: %s\n", recv_port);

        sleep(1);
        if ((sec_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("Error opening socket!\n");
            return -1;
        }

        memset(&sec_servaddr, 0, sizeof(sec_servaddr));
        sec_servaddr.sin_family = AF_INET;
        if (DEBUG)
            printf("port int is %d\n", atoi(recv_port));
        sec_servaddr.sin_port = htons(atoi(recv_port));

        if (inet_pton(AF_INET, ipinput, &sec_servaddr.sin_addr) <= 0) {
            printf(
                    "Some known issues were encountered when trying to use IP address.\nThis error has previously occured when nwprog server was used for the client.\nPlease use other computer for the client!\n");
            printf("inet_pton error\n");
            return -1;
        }

        // Connect to IP address and port indicated by sec_servaddr
        // Check if it was succesful
        if (connect(sec_sockfd, (struct sockaddr *) &sec_servaddr,
                sizeof(sec_servaddr)) < 0) {
            perror("TCP connect error!\n");
            return -1;
        }
        if (DEBUG)
            printf("Connected\n");

        printf("Give filename to upload: ");
        scanf("%s", filename);
        //-------------
        //file transfer
        //-------------

        FILE *filetosend = fopen(filename, "rb");
        if (filetosend == NULL) {
            printf("Couldn't open the file.\n");
            printf("Closing client\n");
            return -1;
        }
        if (DEBUG)
            printf("Sending filename to the server...\n");
        write(sec_sockfd, filename, strlen(filename) + 1);
        sleep(1);
        count = 0;
        printf("Uploading file. Please wait...\n");
        while (1) {
            //read data, 512 bytes at a time
            unsigned char filebuffer[512] = { 0 };
            //printf("starting to read\n");
            int bytes_read = fread(filebuffer, 1, 512, filetosend);
            //printf("read %d bytes\n", bytes_read);

            //sending the data
            if (bytes_read > 0) {
                //printf("writing chunk to server\n");
                write(sec_sockfd, filebuffer, bytes_read);
                count = count + bytes_read;
            }

            //find eof
            if (bytes_read < 512) {
                if (feof(filetosend))
                    if (DEBUG)
                        printf("End of file reached\n");

                if (ferror(filetosend))
                    printf("Error reading the file\n");
                break;
            }
        }
        sleep(1);
        printf("File uploaded successfully (%d bytes)\n", count);
        //printf("data transfer complete\n");
        //close(sec_sockfd);
        break;

    case 3:
        printf("\n==UDP SEND==\n");

        if (DEBUG)
            printf(">>>>>> TEMP: Calling udpsendcli(28000, %s).\n", ipinput);
        read(sockfd, recv_port, 160);
        if (DEBUG)
            printf("Server wants connection to port: %s\n", recv_port);
        int tempport = atoi(recv_port);
        char tempport_str[80];
        sprintf(tempport_str, "%d", tempport);
        //char* tempport = "28000";
        //udpsendcli(tempport, ipinput);
        udpsendcli(tempport_str, ipinput);

        break;

    case 4:
        printf("\n==TCP GET==\n");
        read(sockfd, recv_port, 160);
        if (DEBUG)
            printf("Server wants connection to port: %s\n", recv_port);

        sleep(1);
        if ((sec_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("Error opening socket!\n");
            return -1;
        }

        memset(&sec_servaddr, 0, sizeof(sec_servaddr));
        sec_servaddr.sin_family = AF_INET;
        if (DEBUG)
            printf("port int is %d\n", atoi(recv_port));
        sec_servaddr.sin_port = htons(atoi(recv_port));
        //sec_servaddr.sin_port = htons(server_port2);

        //TODO: inet_pton fails for some reason if using ipinput

        if (inet_pton(AF_INET, ipinput, &sec_servaddr.sin_addr) <= 0) {
            printf(
                    "Some known issues (mainly when using nwprog) were found when trying to use IP address.\nPlease use other computer for the client!\n");
            printf("inet_pton error\n");
            return -1;
        }

        // Connect to IP address and port indicated by sec_servaddr
        // Check if it was succesful
        if (connect(sec_sockfd, (struct sockaddr *) &sec_servaddr,
                sizeof(sec_servaddr)) < 0) {
            perror("TCP connect error!\n");
            return -1;
        }
        if (DEBUG)
            printf("Connected\n");

        //-------------
        //file transfer
        //-------------
        printf("Give filename to download: ");
        scanf("%s", filename);

        //read(sec_sockfd, filebuffer, 512)
        if (DEBUG)
            printf("Opening file\n");
        FILE *newfile;
        newfile = fopen(filename, "wb");
        if (newfile == NULL) {
            printf("Error opening local file");
            return -1;
        }

        write(sec_sockfd, filename, strlen(filename) + 1);

        //TODO If file isn't found from server, should return -1;
        // We need to implement some kind of OK/NOK check from server
        if (DEBUG)
            printf("Waiting for file name check from server\n");
        memset(recv_port, 0, strlen(recv_port));
        read(sec_sockfd, recv_port, 160);
        //printf("%s\n", recv_port);
        if (strcmp(recv_port, "1")) {
            printf("Server doesnt have file: %s\n", filename);
            return -1;
        } else {
            if (DEBUG)
                printf("Server accepted filename: %s\n", filename);
        }
        printf("Downloading file from server. Please wait...\n");
        count = 0;
        //receive data, 512 bytes at a time
        while ((recvbytes = read(sec_sockfd, filebuffer, 512)) > 0) {
            //printf("bytes received: %d\n", recvbytes);    
            fwrite(filebuffer, 1, recvbytes, newfile);
            count = count + recvbytes;
        }
        fclose(newfile);
        printf("File downloaded successfully (%d bytes)\n", count);
        break;

    case 5:
        printf("\n==UDP GET==\n");

        read(sockfd, recv_port, 160);
        if (DEBUG)
            printf("Server wants connection to port: %s\n", recv_port);
        int tempport2 = atoi(recv_port);
        char tempport2_str[80];
        sprintf(tempport2_str, "%d", tempport2);
        udpgetcli(tempport2_str, ipinput);

        break;

    default:
        printf("Got an unknown command, terminating!\n");
        return -1;
    }
    printf("Closing client\n");
    close(sockfd);
    return 0;
}
