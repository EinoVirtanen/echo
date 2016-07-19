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
#include "filelist.h"

#define DEBUG 0 // This can be changed to 0 in normal usage
char filelisting[(NAME_MAX + 1) * 10000 + 16]; // Limits the file listing to 10 000 files

const char* filelist() {

    if (DEBUG)
        printf("Running 'filelist.c'.\n");

    strcpy(filelisting, "List of files:\n");
    DIR *directory;
    struct dirent *ent;

    if (DEBUG)
        printf("Calling 'opendir'.\n");

    if ((directory = opendir("storage")) != NULL) {

        if (DEBUG)
            printf("Calling 'readdir'.\n");

        while ((ent = readdir(directory)) != NULL) {

            if (DEBUG)
                printf("strcat(filelisting, ent->d_name);\n");
            if (strcmp(ent->d_name, "..") && strcmp(ent->d_name, ".")) {
                strcat(filelisting, ent->d_name);
                strcat(filelisting, "\n");
            }
        }

        closedir(directory);

    } else {
        printf("Failed to read 'storage' directory.");
    }

    printf("'filelisting':\n%s\n", filelisting);

    return filelisting;
}
