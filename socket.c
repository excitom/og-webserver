/**
 * This is a collection of socket I/O-related functions.
 *
 * (c) Tom Lang 2/2023
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include "server.h"
#include "global.h"

/**
 * Create a socket for listening, bind it to an address and port,
 * and start listening.
 *
 * Returns: socket file descriptor
 */
int
createBindAndListen(int port)
{
	char buff[BUFF_SIZE];
	char* buffer = (char *)&buff;

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		fprintf(stderr, "Could not create new socket: %m\n");
		exit(1);
	}
	fprintf(stderr, "New socket created with sockfd %d\n", sockfd);

	// use blocking I/O for TLS until for now
	if (!g.useTLS) {
		if (fcntl(sockfd, F_SETFL, O_NONBLOCK)) {
			fprintf(stderr, "Could not make the socket non-blocking: %m\n");
			close(sockfd);
			exit(1);
		}
	}

	int on = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on))) {
		fprintf(stderr, "Could not set socket %d option for reusability: %m\n", sockfd);
		close(sockfd);
		exit(1);
	}

	struct sockaddr_in bindaddr;
	bzero(&bindaddr, sizeof(bindaddr));
	bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bindaddr.sin_family= AF_INET;
	bindaddr.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *) &bindaddr, sizeof(struct sockaddr_in)) < 0) {
		fprintf(stderr, "Could not bind socket %d to address 'INADDR_ANY' and port %u: %m", sockfd, port);
		close(sockfd);
		exit(1);
	} else {
		snprintf(buffer, BUFF_SIZE, "Bound socket %d to address 'INADDR_ANY' and port %u\n", sockfd, port);
		doDebug(buffer);
	}

	if (listen(sockfd, SOMAXCONN)) {
		snprintf(buffer, BUFF_SIZE, "Could not start listening on server socket %d: %m\n", sockfd);
		doDebug(buffer);
		cleanup(sockfd);
	} else {
		snprintf(buffer, BUFF_SIZE, "Server socket %d started listening to address 'INADDR_ANY' and port %u\n", sockfd, port);
		doDebug(buffer);
	}
	return sockfd;
}

/**
 * Receive data from a socket.
 */
int
recvData(int fd, char* ptr, int nbytes)
{
	char *p = ptr;
	int n = 0;
	int received = 0;
	while ((n = recv(fd, p, nbytes, 0)) < 0) {
		if (errno != EINTR) {
			fprintf(stderr, "Receive from socket %d failed: %m\n", fd);
			shutdown(fd, SHUT_RDWR);
			close(fd);
			return(0);
		}
		nbytes -= n;		// reduce buffer size by amount received
		received += n;	  // increment amount received
		p += n;			 // advance the buffer pointer past received data
	}
	received += n;
	doTrace( 'R', ptr, received);
	return received;
}

/**
 * Send data to a socket
 */
int
sendData(int fd, SSL *ssl, char* ptr, int nbytes)
{
	doTrace( 'S', ptr, nbytes);
	size_t nsent;
	if (g.useTLS) {
		if (SSL_write_ex(ssl, ptr, nbytes, &nsent) == 0) {
			if (g.debug) {
				ERR_print_errors_fp(stderr);
			}
		}
	} else {
		int nleft = nbytes;
		while (nleft > 0) {
			nsent = send(fd, ptr, nleft, 0);
			if ((int)nsent > nleft)
				return -1;
			if (nsent > 0) {
				nleft -= nsent;
				ptr   += nsent;
			}
			else if (!((int)nsent == -1 && errno != EINTR)) {
				fprintf(stderr, "Send to socket %d failed: %m\n", fd);
			}
		}
	}
	return nsent;
}

/*
 * Cleanp listening socket and exit
 */
void
cleanup(int sockfd)
{
	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);
	exit(0);
}
