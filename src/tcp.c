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


#define LISTENQ 5
#define DEBUG 0 // This can be changed to 0 in normal usage



int tcpget(int sockfd, int port) {
	struct sockaddr_in6 servaddr2, cliaddr;
	int listenfd, connfd;
	socklen_t len;
	char buff[80];
	printf("starting new tcp socket\n");
	sprintf(buff, "%d", port);
	write(sockfd, buff, strlen(buff));
	printf(">> %s\n", buff);
	// create socket for listening
	if ((listenfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return -1;
	}
	// Pick a port and bind socket to it.
	// Accept connections from any address.
	memset(&servaddr2, 0, sizeof(servaddr2));
	servaddr2.sin6_family = AF_INET6;
	servaddr2.sin6_addr = in6addr_any;
	servaddr2.sin6_port = htons(port);
	if (bind(listenfd, (struct sockaddr *) &servaddr2, sizeof(servaddr2)) < 0) {
		printf("problem in bind...\n");
		perror("bind");
		return -1;
	}
	// Set the socket to passive mode, with specified listen queue size
	if (listen(listenfd, LISTENQ) < 0) {
		perror("listen");
		return -1;
	}
	// Repeatedly wait for incoming connections
	len = sizeof(cliaddr);
	// wait for incoming connection
	// new socket fd will be used in return
	if ((connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len))
	< 0) {
		perror("accept");
		return -1;
	}
	printf("tcp server: connection from %s, port %d\n",
	inet_ntop(AF_INET6, &cliaddr.sin6_addr, buff, sizeof(buff)),
	ntohs(cliaddr.sin6_port));
	////////////////////////////////
	//file transfer implementation//
	////////////////////////////////
	char filename_buff[160];
	printf("Reading filename from client\n");
	read(connfd, filename_buff, sizeof(filename_buff));
	printf("Got filename: %s\n", filename_buff);

	printf("Ready to send...\n");
	char filepath[160] = "storage/";
	strcat(filepath, filename_buff);
	FILE *filetosend = fopen(filepath, "rb");
	if (filetosend == NULL) {
		printf("couldn't open the file\n"); // TODO: if filename not in folder
		write(connfd, "0", strlen("0"));	// write(sockfd, "NOK", strlen("NOK"));
		return -1;
	} else {
		write(connfd, "1", strlen("1"));
	}
	int count = 0;
	sleep(1);
	//counts total bytes for final print
	while (1) {
		//read data, 512 bytes at a time
		unsigned char filebuffer[512] = {0};
		int bytes_read = fread(filebuffer, 1, 512, filetosend);
		//printf("read %d bytes\n", bytes_read);
		//sending the data
		if (bytes_read > 0) {
			write(connfd, filebuffer, bytes_read);
			count = count + bytes_read;
		}
		//find eof
		if (bytes_read < 512) {
			if (feof(filetosend))
			printf("End of file reached\n");
			if (ferror(filetosend))
			printf("Error reading the file\n");
			break;
		}
	}
	sleep(1);
	printf("File sent (%d bytes)\n", count);
	printf("data transfer complete\n");
	close(connfd);
	printf("closing sockets in child process\n");
	close(sockfd);
	return 0;
}
int tcpsend(int sockfd, int port) {
	int recvbytes;
	struct sockaddr_in6 servaddr2, cliaddr;
	// TODO: if filename not in folder
	//   write(sockfd, "NOK", strlen("NOK"));
	//
	//
	int listenfd, connfd;
	socklen_t len;
	char buff[80];
	char filebuffer[512];
	sprintf(buff, "%d", port);
	write(sockfd, buff, strlen(buff));
	printf(">> %s\n", buff);
	printf("starting new tcp socket\n");
	// create socket for listening
	if ((listenfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return -1;
	}
	// Pick a port and bind socket to it.
	// Accept connections from any address.
	memset(&servaddr2, 0, sizeof(servaddr2));
	servaddr2.sin6_family = AF_INET6;
	servaddr2.sin6_addr = in6addr_any;
	servaddr2.sin6_port = htons(port);
	if (bind(listenfd, (struct sockaddr *) &servaddr2, sizeof(servaddr2))
	< 0) {
		printf("problem in bind...\n");
		perror("bind");
		return -1;
	}
	// Set the socket to passive mode, with specified listen queue size
	if (listen(listenfd, LISTENQ) < 0) {
		perror("listen");
		return -1;
	}
	// Repeatedly wait for incoming connections
	len = sizeof(cliaddr);
	// wait for incoming connection
	// new socket fd will be used in return
	if ((connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len)) < 0) {
		perror("accept");
		return -1;
	}
	printf("tcp server: connection from %s, port %d\n",
	inet_ntop(AF_INET6, &cliaddr.sin6_addr, buff, sizeof(buff)),
	ntohs(cliaddr.sin6_port));
	char filename_buff[160];
	printf("Reading filename from client\n");
	read(connfd, filename_buff, sizeof(filename_buff));
	printf("Got filename: %s\n", filename_buff);
	char filepath[160] = "storage/";
	strcat(filepath, filename_buff);
	//file transfer implementation
	FILE *newfile;
	newfile = fopen(filepath, "wb");
	if (newfile == NULL) {
		printf("Error opening file");
		return -1;
	}
	printf("Ready to receive...\n");
	int count = 0;
	//receive data, 512 bytes at a time
	while ((recvbytes = read(connfd, filebuffer, 512)) > 0) {
		//printf("bytes received: %d\n", recvbytes);
		fwrite(filebuffer, 1, recvbytes, newfile);
		count = count + recvbytes;
	}
	fclose(newfile);
	printf("File received (%d bytes)\n", count);
	printf("data transfer complete\n");
	close(connfd);
	printf("closing sockets in child process\n");
	close(sockfd);
	printf("Closed sockets in tcpget.\n");
	return 0;
}
