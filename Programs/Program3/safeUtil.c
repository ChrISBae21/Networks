
// 
// Writen by Hugh Smith, April 2020, Feb. 2021
//
// Put in system calls with error checking
// keep the function paramaters same as system call

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "safeUtil.h"

#ifdef __LIBCPE464_
#include "cpe464.h"
#endif




int safeSocket() {
	int socketNum;
	if ((socketNum = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
			perror("socket() call error");
			exit(-1);
	}
	return socketNum;
}

int safeRecvfrom(int socketNum, void * buf, int len, int flags, struct sockaddr *srcAddr, int * addrLen) {
	int returnValue = 0;
	if ((returnValue = recvfrom(socketNum, buf, (size_t) len, flags, srcAddr, (socklen_t *) addrLen)) < 0) {
		perror("recvfrom: ");
		exit(-1);
	}
	
	return returnValue;
}

int safeSendto(int socketNum, void * buf, int len, int flags, struct sockaddr *srcAddr, int addrLen) {
	int returnValue = 0;
	// if ((returnValue = sendto(socketNum, buf, (size_t) len, flags, srcAddr, (socklen_t) addrLen)) < 0) {
	if ((returnValue = sendtoErr(socketNum, buf, (size_t) len, flags, srcAddr, (socklen_t) addrLen)) < 0) {
		perror("sendto: ");
		exit(-1);
	}
	return returnValue;
}

void * srealloc(void *ptr, size_t size) {
	void * returnValue = NULL;
	
	if ((returnValue = realloc(ptr, size)) == NULL) {
		printf("Error on realloc (tried for size: %d\n", (int) size);
		exit(-1);
	}
	
	return returnValue;
} 

void * sCalloc(size_t nmemb, size_t size) {
	void * returnValue = NULL;
	if ((returnValue = calloc(nmemb, size)) == NULL) {
		perror("calloc");
		exit(-1);
	}
	return returnValue;
}

