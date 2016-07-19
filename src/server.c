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
#include <signal.h>
#include <sys/wait.h>
#include "tcp.h"
#include "filelist.h"
#include "portlist.h"
#include "udpsend.h"
#include "udpget.h"

#define LISTENQ 5

#define DEBUG 0 // This can be changed to 0 in normal usage

int main(int argc, char **argv) {

    printf(
            "Running 'echo' file transfer server. Make sure this directory has 'storage' directory for file storage.\n");

    if (DEBUG)
        printf("Debug mode on.\n");

    int sockfd, m, cmd, portCandidate;
    char *sendbuffer, commandbuffer[160];
    size_t sendbufferusage = 0;
    pid_t childpid;
    struct sockaddr_in6 servaddr2, cliaddr;
    struct portnode *portListRoot;
    portListRoot = (struct portnode *) malloc(sizeof(struct portnode));
    portListRoot->port = 6666;
    portListRoot->next = NULL;
    ///////
    //SERVER SOCKET
    ///////
    int listenfd, connfd;
    socklen_t len;
    char buff[80];
    const int on = 1;

    // create socket for listening
    if ((listenfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    // Pick a port and bind socket to it.
    // Accept connections from any address.
    memset(&servaddr2, 0, sizeof(servaddr2));
    servaddr2.sin6_family = AF_INET6;
    //servaddr2.sin6_addr = in6addr_any;
    servaddr2.sin6_addr = in6addr_any;
    servaddr2.sin6_port = htons(6666);

    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

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
    if(DEBUG)
    printf("Main process ID: %d\n", getpid());
    
    printf("Listening for incoming connections....\n");

    //signalling to prevent zombie processes when childs exit
    struct sigaction sigchld_action = { .sa_handler = SIG_DFL, .sa_flags =
    SA_NOCLDWAIT };
    sigaction(SIGCHLD, &sigchld_action, NULL);

    // Repeatedly wait for incoming connections
    for (;;) {
        len = sizeof(cliaddr);

        // wait for incoming connection
        // new socket fd will be used in return
        if ((connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len))
                < 0) {
            perror("accept");
            return -1;
        }

        printf("\nNew connection from %s, port %d\n",
                inet_ntop(AF_INET6, &cliaddr.sin6_addr, buff, sizeof(buff)),
                ntohs(cliaddr.sin6_port));

        //MULTI USER HANDLING
        //function fork() creates new child process for every incoming client
        //fork returns 0 for child process, pid for main process
        //After child process is complete, it's closed.
        //Main process continues to run until killed
        //port = 49153; //Setting the initial port from the "unallocated" port space for child processes
        for (portCandidate = 49153; portCandidate < 60000; portCandidate++) {
	    if(DEBUG)
            printf("port candidate: %d\n", portCandidate);
            if (!portUsed(portListRoot, portCandidate)) {
                addUsedPort(portListRoot, portCandidate);
                break;
            }
        }
        if (DEBUG)
            printUsedPorts(portListRoot);

        childpid = fork();
        if (childpid < 0) {
            perror("ERROR on fork");
            exit(1);
        }

        if (childpid > 0) {
            //We are now in the main process
	    if(DEBUG)
            printf("%d: Child process created -> %d\n", getpid(), childpid);
            //connfd socket was now copied to the child process so we can kill it in the main process
            close(connfd);
        }

        if (childpid == 0) {
            //We are now in the child process. Child doesn't need listen socket so we can close it
	    if(DEBUG)
            printf("child process %d: closing listenfd...\n", getpid());
            close(listenfd);

            printf("Reading choice from the client\n");
            while (m = read(connfd, commandbuffer, sizeof(commandbuffer)) > 0) { // "Possible assignment in condition 'm = read(connfd, commandbuffer, sizeof(commandbuffer)) > 0'   server.c    /echo/src   line 84 Semantic Error"

                if (DEBUG)
                    printf("Got '%d' from the client.\n", commandbuffer[0]);

                cmd = commandbuffer[0];

                switch (cmd) {
                case 1:
                    printf("Client sent 'FILELIST' command. We have:\n");

                    char filelisting[(NAME_MAX + 1) * 10000 + 16];
                    strcpy(filelisting, filelist());
                    if (DEBUG)
                        printf("We have:\n%s", filelisting);
                    if (write(connfd, filelisting, strlen(filelisting)) == -1) {
                        perror("Sending data failed");
                    } else {
                        printf("Sent filelist to client.\n");
                    }
                    //return 0; // remember to remove this

                    break;

                case 2:
                    printf("Client sent 'TCPSEND' command.\n");
                    tcpsend(connfd, portCandidate);

                    break;

                case 3:
                    printf("Client sent 'UDPSEND' command.\n");
                    char buff[80];
                    sprintf(buff, "%d", portCandidate);
                    write(connfd, buff, strlen(buff));
		    if(DEBUG)
                    printf(">> %s\n", buff);

                    udpsend(portCandidate);
                    break;

                case 4:
                    printf("Client sent 'TCPGET' command.\n");
                    tcpget(connfd, portCandidate);

                    break;

                case 5:
                    printf("Client sent 'UDPGET' command.\n");

                    char buff2[80];
                    sprintf(buff2, "%d", portCandidate);
                    write(connfd, buff2, strlen(buff2));
		    if(DEBUG)
                    printf(">> %s\n", buff2);
                    udpget(portCandidate);
                    break;

                default:
                    printf("Got an unknown command, terminating!\n");
                    break;
                }
                //only child process can be here. removing port reservation and exiting child process.
                printf("Closing child process (pid: %d).\n", getpid());
		printf("Listening for more incoming connections\n");
                //removeUsedPort(portListRoot, portCandidate); CALLED IN CHILD -> NO USE
                exit(0);

            }

        }

        printf("Child process created. Listening for more incoming connections\n");
    }

    //close(listenfd);

    //while//}

    close(sockfd);
    return 0;
}
