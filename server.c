/**
 * Tom's OG Web Server
 *
 * This is a simple web server which is implemented with an event driven
 * model. There is a single process and a no threads, but everything is
 * done with non-blocking, asynchronous I/O.
 *
 * Tom Lang 2/2023
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
#include <sys/epoll.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "server.h"

// global variables
struct globalVars g;

char* buffer;
int fdCount = 1;

/**
 * MAIN function
 */
int
main(int argc, char *argv[])
{
	buffer = malloc(BUFF_SIZE);

	parseArgs(argc, argv);
	daemonize();
	parseMimeTypes();

	int epollfd = epollCreate();

	int sockfd = createBindAndListen(g.port);

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.u64 = 0LL;
	ev.data.fd = sockfd;

	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
		snprintf(buffer, BUFF_SIZE, "Couldn't add server socket %d to epoll set: %m\n", sockfd);
		doDebug(buffer);
		cleanup(sockfd);	  
	}

	if (g.useTLS) {
		ctx = create_context();
		configure_context(ctx);
		SSL_load_error_strings();
		ERR_load_crypto_strings();
	}

	//
	// Main event loop
	//
	while(1) {
		snprintf(buffer, BUFF_SIZE, "Starting epoll_wait on %d fds\n", fdCount);
		doDebug(buffer);

		int rval;
		struct epoll_event epoll_events[g.epollArraySize];
		//
		// Loop if interrupted by a signal
		//
		while ((rval = epoll_wait(epollfd, epoll_events, g.epollArraySize, -1)) < 0) {
			if ((rval < 0) && (errno != EINTR)) {
				snprintf(buffer, BUFF_SIZE, "EPoll on %d fds failed: %m\n", fdCount);
				doDebug(buffer);
				cleanup(sockfd);
			}
		}

		//
		// Loop over returned events
		//
		for (int i = 0; i < rval; i++) {
			int clientsfd;	  // descriptor for client connection
			uint32_t events;	// events map
			events = epoll_events[i].events;
			int fd = epoll_events[i].data.fd;

			//
			// Misc error
			//
			if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
				if (fd == sockfd) {
					snprintf(buffer, BUFF_SIZE, "EPoll on %d fds failed: %m\n", fdCount);
					doDebug(buffer);
					cleanup(sockfd);
				} else {
					snprintf(buffer, BUFF_SIZE, "Closing socket with sockfd %d\n", fd);
					doDebug(buffer);
					shutdown(fd, SHUT_RDWR);
					close(fd);
					continue;
				}
			}

			//
			// There is data from a socket
			//
			if (events & EPOLLIN) {
				//
				// Input on the listening socket means a new incoming connection
				//
				if (fd == sockfd) {
					struct sockaddr_in peeraddr;
					socklen_t salen = sizeof(peeraddr);
					while ((clientsfd = accept(sockfd, (struct sockaddr *) &peeraddr, &salen)) < 0) {
						if ((clientsfd < 0) && (errno != EINTR)) {
							snprintf(buffer, BUFF_SIZE, "Accept on socket %d failed: %m\n", sockfd);
							doDebug(buffer);
							cleanup(sockfd);
						}
					}

					SSL *ssl = NULL;
					if (g.useTLS) {
						ssl = SSL_new(ctx);
						SSL_set_fd(ssl, clientsfd);
						if (SSL_accept(ssl) <= 0) {
							perror("SSL accept failed ");
							cleanup(sockfd);
						}
					}

					if (g.debug) {
						char ipinput[INET_ADDRSTRLEN];
						if (inet_ntop(AF_INET, &peeraddr.sin_addr.s_addr, ipinput, INET_ADDRSTRLEN) != NULL) {
							snprintf(buffer, BUFF_SIZE, "Accepted connection from %s:%u, assigned new sockfd %d\n", ipinput, ntohs(peeraddr.sin_port), clientsfd);
							doDebug(buffer);
						} else {
							snprintf(buffer, BUFF_SIZE, "Failed to convert address from binary to text form: %m\n");
							doDebug(buffer);
						}
					}

					//
					// Add a new event to listen for
					//
					ev.events = EPOLLIN;
					ev.data.u64 = 0LL;
					ev.data.fd = clientsfd;	

					if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientsfd, &ev) < 0) {
						snprintf(buffer, BUFF_SIZE, "Couldn't add client socket %d to epoll set: %m\n", clientsfd);
						doDebug(buffer);
						cleanup(sockfd);	  
					}

					fdCount++;
				} else {
					//
					// Process the incoming data from a socket
					//
					processInput(fd, ssl);

					// not handling "keep alive" yet
					if (g.useTLS) {
						SSL_shutdown(ssl);
						SSL_free(ssl);
					}
					shutdown(fd, SHUT_RDWR);
					close(fd);
					fdCount--;
				}
			} // End, process an event
		} // End, loop over returned events
	} // End, main event loop
}

/**
 * Create an epoll file descriptor for waiting on events
 *
 * Returns: file descriptor
 */
int
epollCreate()
{

	int pollsize = 1;   // deprecated parameter, but must be > 0
	int epollfd = epoll_create(pollsize);

	if (epollfd < 0) {
		snprintf(buffer, BUFF_SIZE, "Could not create the epoll fd: %m");
		doDebug(buffer);
		exit(1);
	}
	snprintf(buffer, BUFF_SIZE, "epoll fd created successfully");
	doDebug(buffer);
	return epollfd;
}

/**
 * Create a socket for listening, bind it to an address and port,
 * and start listening.
 *
 * Returns: socket file descriptor
 */
int
createBindAndListen(int port)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		snprintf(buffer, BUFF_SIZE, "Could not create new socket: %m\n");
		doDebug(buffer);
		exit(1);
	}
	snprintf(buffer, BUFF_SIZE, "New socket created with sockfd %d\n", sockfd);
	doDebug(buffer);
	if (fcntl(sockfd, F_SETFL, O_NONBLOCK)) {
		snprintf(buffer, BUFF_SIZE, "Could not make the socket non-blocking: %m\n");
		doDebug(buffer);
		close(sockfd);
		exit(1);
	}

	int on = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
		snprintf(buffer, BUFF_SIZE, "Could not set socket %d option for reusability: %m\n", sockfd);
		doDebug(buffer);
		close(sockfd);
		exit(1);
	}

	struct sockaddr_in bindaddr;
	bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bindaddr.sin_family= AF_INET;
	bindaddr.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *) &bindaddr, sizeof(struct sockaddr_in)) < 0) {
		snprintf(buffer, BUFF_SIZE, "Could not bind socket %d to address 'INADDR_ANY' and port %u: %m", sockfd, port);
		doDebug(buffer);
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
recvData(int fd, unsigned char* ptr, int nbytes)
{
	unsigned char *p = ptr;
	int n = 0;
	int received = 0;
	while ((n = recv(fd, p, nbytes, 0)) < 0) {
		if (errno != EINTR) {
			snprintf(buffer, BUFF_SIZE, "Receive from socket %d failed: %m\n", fd);
			doDebug(buffer);
			fdCount--;
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
sendData(int fd, unsigned char* ptr, int nbytes)
{
	doTrace( 'S', (unsigned char *)ptr, nbytes);
	int nleft = nbytes;
	while (nleft > 0) {
		int nsent = send(fd, ptr, nleft, 0);
		if (nsent > nleft)
			return -1;
		if (nsent > 0) {
			nleft -= nsent;
			ptr   += nsent;
		}
		else if (!(nsent == -1 && errno != EINTR)) {
			snprintf(buffer, BUFF_SIZE, "Send to socket %d failed: %m\n", fd);
			doDebug(buffer);
			fdCount--;
			shutdown(fd, SHUT_RDWR);
			close(fd);
			return nsent;
		}
	}
	return nbytes;
}

/**
 * Create SSL context
 */
SSL_CTX *
create_context()
{
	const SSL_METHOD *method;
	SSL_CTX *ctx;

	method = TLS_server_method();

	ctx = SSL_CTX_new(method);
	if (!ctx) {
		perror("Unable to create SSL context");
		exit(1);
	}

	return ctx;
}

/**
 * Configure SSL
 */
void
configure_context(SSL_CTX *ctx)
{
	/* Set the key and cert */
	if (SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0) {
		perror("Unable to use SSL certificate ");
		exit(1);
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) <= 0 ) {
		perror("Unable to use SSL private key ");
		exit(EXIT_FAILURE);
	}
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
