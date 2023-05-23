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
createBindAndListen(int isTLS, int port)
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
	if (!isTLS) {
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
	memset(ptr, 0, nbytes);
	while ((n = recv(fd, p, nbytes, 0)) < 0) {
		if (errno == EAGAIN) {
			return 0;
		}
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
	if (ssl) {
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

/**
 * Keep track of client connections
 */
_clientConnection *
queueClientConnection(int fd, struct sockaddr_in addr, SSL_CTX *ctx)
{
	_clientConnection *client = (_clientConnection *)malloc(sizeof(_clientConnection));
	client->fd = fd;
	client->ctx = ctx;
	char ip[INET_ADDRSTRLEN];
	if (inet_ntop(AF_INET, &addr.sin_addr.s_addr, ip, INET_ADDRSTRLEN) != NULL) {
		char buffer[BUFF_SIZE];
		snprintf(buffer, BUFF_SIZE, "Accepted connection from %s:%u, assigned new sockfd %d\n", ip, ntohs(addr.sin_port), fd);
		doDebug(buffer);
		strncpy(client->ip, ip, INET_ADDRSTRLEN);
	} else {
		perror("Failed to convert address from binary to text form");
		exit(1);
	}
	_clientConnection *c = g.clients;
	client->next = c;
	g.clients = client;
	return client;
}

/**
 * Cleanup listening socket
 */
void
cleanup(int fd)
{
	_clientConnection *c = g.clients;
	_clientConnection *prev = NULL;
	while(c) {
		if (c->fd == fd) {
			if (prev) {
				prev->next = c->next;
			} else {
				g.clients = c->next;
			}
			break;
		}
		prev = c;
		c = c->next;
	}
	if (c == NULL) {
		doDebug("Missing client connection!");
		exit(1);
	}
	if (c->ctx) {
		SSL_CTX_free(c->ctx);
	}
	shutdown(fd, SHUT_RDWR);
	close(fd);
	free(c);
	return;
}

/**
 * Find client connection info for socket
 */
_clientConnection *
getClient(int fd)
{
	_clientConnection *c = g.clients;
	while(c) {
		if (c->fd == fd) {
			break;
		}
		c = c->next;
	}
	if (c ==NULL) {
		doDebug("Missing client connection!");
		exit(1);
	}
	return c;
}
