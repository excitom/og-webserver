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
#include <sys/sendfile.h>
#include <arpa/inet.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include "serverlist.h"
#include "server.h"

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

	int sockFd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockFd < 0) {
		fprintf(stderr, "Could not create new socket: %m\n");
		exit(1);
	}
	fprintf(stderr, "New socket created with sockFd %d\n", sockFd);

	// use blocking I/O for TLS until for now
	if (!isTLS) {
		if (fcntl(sockFd, F_SETFL, O_NONBLOCK)) {
			fprintf(stderr, "Could not make the socket non-blocking: %m\n");
			close(sockFd);
			exit(1);
		}
	}

	int on = 1;
	if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on))) {
		fprintf(stderr, "Could not set socket %d option for reusability: %m\n", sockFd);
		close(sockFd);
		exit(1);
	}

	struct sockaddr_in bindaddr;
	bzero(&bindaddr, sizeof(bindaddr));
	bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bindaddr.sin_family= AF_INET;
	bindaddr.sin_port = htons(port);

	if (bind(sockFd, (struct sockaddr *) &bindaddr, sizeof(struct sockaddr_in)) < 0) {
		fprintf(stderr, "Could not bind socket %d to address 'INADDR_ANY' and port %u: %m", sockFd, port);
		close(sockFd);
		exit(1);
	} else {
		snprintf(buffer, BUFF_SIZE, "Bound socket %d to address 'INADDR_ANY' and port %u\n", sockFd, port);
		doDebug(buffer);
	}

	if (listen(sockFd, SOMAXCONN)) {
		snprintf(buffer, BUFF_SIZE, "Could not start listening on server socket %d: %m\n", sockFd);
		doDebug(buffer);
		cleanup(sockFd);
	} else {
		snprintf(buffer, BUFF_SIZE, "Server socket %d started listening to address 'INADDR_ANY' and port %u\n", sockFd, port);
		doDebug(buffer);
	}
	return sockFd;
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
sendData(int fd, SSL *ssl, const char* ptr, int nbytes)
{
	doTrace( 'S', ptr, nbytes);
	size_t nsent;
	if (ssl) {
		if (SSL_write_ex(ssl, ptr, nbytes, &nsent) == 0) {
			if (isDebug()) {
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
 * Copy a file to a socket
 */
void
sendFile(_request *req, size_t size)
{
	if (isDebug()) {
		fprintf(stderr, "Sending response body: SIZE %d\n", (int)size);
	}
	off_t offset = 0;
	size_t sent;
	if (req->ssl) {
		char *p = malloc(size);
		read(req->localFd, p, size);
		if (SSL_write_ex(req->ssl, p, size, &sent) == 0) {
			if (isDebug()) {
				ERR_print_errors_fp(stderr);
			}
		}
		free(p);
	} else {
		sent = sendfile(req->clientFd, req->localFd, &offset, size);
	}
	if (sent != size) {
		if (isDebug()) {
			fprintf(stderr, "Problem sending response body: SIZE %d SENT %d\n", (int)size, (int)sent);
		}
	}
}

/**
 * Keep track of client connections
 */
_clientConnection *
queueClientConnection(int fd, int errorFd, struct sockaddr_in addr, SSL_CTX *ctx)
{
	_clientConnection *client = (_clientConnection *)malloc(sizeof(_clientConnection));
	client->fd = fd;
	client->errorFd = errorFd;
	client->ctx = ctx;
	char ip[INET_ADDRSTRLEN];
	if (inet_ntop(AF_INET, &addr.sin_addr.s_addr, ip, INET_ADDRSTRLEN) != NULL) {
		char buffer[BUFF_SIZE];
		snprintf(buffer, BUFF_SIZE, "Accepted connection from %s:%u, assigned new sockFd %d\n", ip, ntohs(addr.sin_port), fd);
		doDebug(buffer);
		strncpy(client->ip, ip, INET_ADDRSTRLEN);
	} else {
		perror("Failed to convert address from binary to text form");
		exit(1);
	}
	setClientConnection(client);
	return client;
}

/**
 * Cleanup listening socket
 */
void
cleanup(int fd)
{
	_clientConnection *c = removeClientConnection(fd);
	if (c) {
		if (c->ctx) {
			SSL_CTX_free(c->ctx);
		}
		free(c);
	}
	shutdown(fd, SHUT_RDWR);
	close(fd);
	return;
}

/**
 * Find client connection info for socket
 */
_clientConnection *
getClient(int fd)
{
	_clientConnection *c = getClientConnection(fd);
	if (c == NULL) {
		doDebug("Missing client connection!");
		exit(1);
	}
	return c;
}
