#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L // Newest POSIX definition for everything
#endif
int compareFileSize(int size1, int size2){
	if (size1 == size2)
		return 1;
	return 0;
}

#include "filecheck.h"

