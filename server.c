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
#include "server.h"

// global variables
struct globalVars g;

unsigned char buffer[BUFF_SIZE];
int fdCount = 1;

/**
 * MAIN function
 */
int
main(int argc, char *argv[])
{
	g.debug = 0;
	g.trace = 0;
	g.port = 8080;

	parseArgs(argc, argv);

	int epollfd = epollCreate();

	int sockfd = createBindAndListen(g.port);

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.u64 = 0LL;
	ev.data.fd = sockfd;

	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
		snprintf(buffer, sizeof(buffer), "Couldn't add server socket %d to epoll set: %m\n", sockfd);
		doDebug(buffer);
		cleanup(sockfd);	  
	}

	//
	// Main event loop
	//
	while(1) {
		snprintf(buffer, sizeof(buffer), "Starting epoll_wait on %d fds\n", fdCount);
		doDebug(buffer);

		int rval;
		struct epoll_event epoll_events[EPOLL_ARRAY_SIZE];
		//
		// Loop if interrupted by a signal
		//
		while ((rval = epoll_wait(epollfd, epoll_events, EPOLL_ARRAY_SIZE, -1)) < 0) {
			if ((rval < 0) && (errno != EINTR)) {
				snprintf(buffer, sizeof(buffer), "EPoll on %d fds failed: %m\n", fdCount);
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
					snprintf(buffer, sizeof(buffer), "EPoll on %d fds failed: %m\n", fdCount);
					doDebug(buffer);
					cleanup(sockfd);
				} else {
					snprintf(buffer, sizeof(buffer), "Closing socket with sockfd %d\n", fd);
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
							snprintf(buffer, sizeof(buffer), "Accept on socket %d failed: %m\n", sockfd);
							doDebug(buffer);
							cleanup(sockfd);
						}
					}

					if (inet_ntop(AF_INET, &peeraddr.sin_addr.s_addr, buffer, sizeof(buffer)) != NULL) {
						snprintf(buffer, sizeof(buffer), "Accepted connection from %s:%u, assigned new sockfd %d\n", buffer, ntohs(peeraddr.sin_port), clientsfd);
						doDebug(buffer);
					} else {
						snprintf(buffer, sizeof(buffer), "Failed to convert address from binary to text form: %m\n");
						doDebug(buffer);
					}

					//
					// Add a new event to listen for
					//
					ev.events = EPOLLIN;
					ev.data.u64 = 0LL;
					ev.data.fd = clientsfd;	

					if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientsfd, &ev) < 0) {
						snprintf(buffer, sizeof(buffer), "Couldn't add client socket %d to epoll set: %m\n", clientsfd);
						doDebug(buffer);
						cleanup(sockfd);	  
					}

					fdCount++;
				} else {
					//
					// Process the incoming data from a socket
					//
					processInput(fd);
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
		snprintf(buffer, sizeof(buffer), "Could not create the epoll fd: %m");
		doDebug(buffer);
		exit(1);
	}
	snprintf(buffer, sizeof(buffer), "epoll fd created successfully");
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
		snprintf(buffer, sizeof(buffer), "Could not create new socket: %m\n");
		doDebug(buffer);
		exit(1);
	}
	snprintf(buffer, sizeof(buffer), "New socket created with sockfd %d\n", sockfd);
	doDebug(buffer);
	if (fcntl(sockfd, F_SETFL, O_NONBLOCK)) {
		snprintf(buffer, sizeof(buffer), "Could not make the socket non-blocking: %m\n");
		doDebug(buffer);
		close(sockfd);
		exit(1);
	}

	int on = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
		snprintf(buffer, sizeof(buffer), "Could not set socket %d option for reusability: %m\n", sockfd);
		doDebug(buffer);
		close(sockfd);
		exit(1);
	}

	struct sockaddr_in bindaddr;
	bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bindaddr.sin_family= AF_INET;
	bindaddr.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *) &bindaddr, sizeof(struct sockaddr_in)) < 0) {
		snprintf(buffer, sizeof(buffer), "Could not bind socket %d to address 'INADDR_ANY' and port %u: %m", sockfd, port);
		doDebug(buffer);
		close(sockfd);
		exit(1);
	} else {
		snprintf(buffer, sizeof(buffer), "Bound socket %d to address 'INADDR_ANY' and port %u\n", sockfd, port);
		doDebug(buffer);
	}

	if (listen(sockfd, SOMAXCONN)) {
		snprintf(buffer, sizeof(buffer), "Could not start listening on server socket %d: %m\n", sockfd);
		doDebug(buffer);
		cleanup(sockfd);
	} else {
		snprintf(buffer, sizeof(buffer), "Server socket %d started listening to address 'INADDR_ANY' and port %u\n", sockfd, port);
		doDebug(buffer);
	}
	return sockfd;
}

/**
 * Parse command line args
 */
void
parseArgs(int argc, char* argv[])
{
	int c;
	while ((c = getopt(argc, argv, "hdtp:")) != EOF)
		switch(c) {
			case 'd':
				g.debug = 1;
				break;
			case 't':
				g.trace = 1;
				break;
			case 'p':
				g.port = atoi(optarg);
				break;
			case 'h':
				printf("Tom's OG web server\n");
				printf("Options are:\n");
				printf("	-d = turn on debugging\n");
				printf("	-t = turn on tracing\n");
				printf("	-h = print this message\n");
				printf("	-p = port on which to listen\n");
				printf("\n");
				exit(0);
			default:
				printf("Unrecognized option %c ignored\n", (char)c);
				break;
		}
}

/**
 * Process input
 */
void
processInput(int fd)
{
	unsigned char inbuff[BUFF_SIZE];
	int received = recvData(fd, (unsigned char *)&inbuff, sizeof(inbuff));
	if (received > 0) {
		snprintf(buffer, sizeof(buffer), "Received %d bytes from fd %d\n", received, fd);
		doDebug(buffer);
		unsigned char ts[TIME_BUF];
		getTimestamp((unsigned char *)&ts);
		int sz = snprintf(buffer, sizeof(buffer), "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nDate: %s\r\n\r\n<HTML><P>Hello socket %d!</P><P>%s</P>\r\n</HTML>\r\n\r\n", ts, fd, ts);
		int sent = sendData(fd, buffer, sz);
	} else {
		// no more data to read
		shutdown(fd, SHUT_RDWR);
		close(fd);
	}
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
			snprintf(buffer, sizeof(buffer), "Receive from socket %d failed: %m\n", fd);
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
sendData(int fd, char* ptr, int nbytes)
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
			snprintf(buffer, sizeof(buffer), "Send to socket %d failed: %m\n", fd);
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
 * Verbose trace
 */
void
doTrace (char direction, unsigned char *p, int bytes)
{
	if (!g.trace)
		return;

	int f, l, i;
	unsigned char d[16];
	f = 0;

	while (bytes >= 16) {
		for (i=0; i<16; i++) {
			d[i] = isprint(p[i]) ? p[i] : '.';
		}
		l = f+15;
		fprintf(stderr, "%4.4X - %4.4X %c %2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X - %2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", f, l, direction, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15], d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
		f += 16;
		p += 16;
		bytes -= 16;
	}
	if (bytes > 0) {
		l = f + bytes - 1;
		fprintf(stderr, "%4.4X - %4.4X %c ", f, l, direction);
		for (i = 0; i < bytes; i++) {
			fprintf(stderr, "%2.2X", p[i]);
			d[i] = isprint(p[i]) ? p[i] : '.';
			if (i == 7)
				fprintf(stderr, " - ");
		}
		for (i = 0; i < 32-(bytes*2); i++)
			fprintf(stderr, " ");
		if (bytes<8)
			fprintf(stderr, "   ");
		fprintf(stderr, "  ");
		for (i = 0; i < bytes; i++) {
			fprintf(stderr, "%c", d[i]);
		}
		fprintf(stderr, "\n");
	}
}

/**
 * Debug print to stderr
 * Note: Assumes a formatted string input.
 */
void doDebug(unsigned char* buffer) {
	if (!g.debug) {
		return;
	}
	fprintf(stderr, buffer);
}

/*
 * Cleanp listening socket and exit
 */
void cleanup(int sockfd)
{
	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);
	exit(0);
}
