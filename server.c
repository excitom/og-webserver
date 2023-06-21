/**
 * This is a simple HTTP web server which is implemented with an event driven
 * model. There is a single process and a no threads, but everything is
 * done with non-blocking, asynchronous I/O.
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
#include <sys/epoll.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <locale.h>
#include "server.h"
#include "global.h"

char buff[BUFF_SIZE];
char* buffer = (char *)&buff;

void
server(int port)
{
	int epollfd = epollCreate();
	const int isTLS = 0;
	int sockfd = createBindAndListen(isTLS, port);
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.u64 = 0LL;
	ev.data.fd = sockfd;

	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
		snprintf(buffer, BUFF_SIZE, "Couldn't add server socket %d to epoll set: %m\n", sockfd);
		doDebug(buffer);
		cleanup(sockfd);	  
		exit(1);
	}

	//
	// Main event loop
	//
	while(1) {
		doDebug("Starting epoll_wait");

		int rval;
		struct epoll_event epoll_events[g.workerConnections];
		//
		// Loop if interrupted by a signal
		//
		while ((rval = epoll_wait(epollfd, epoll_events, g.workerConnections, -1)) < 0) {
			if ((rval < 0) && (errno != EINTR)) {
				doDebug("epoll_wait failed");
				cleanup(sockfd);
				return;
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
					doDebug("epoll_wait failed");
					doDebug(buffer);
					cleanup(sockfd);
					exit(1);
				} else {
					snprintf(buffer, BUFF_SIZE, "Closing socket with sockfd %d\n", fd);
					doDebug(buffer);
					cleanup(fd);
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
							return;
						}
					}
					queueClientConnection(clientsfd, peeraddr, NULL);

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
						exit(1);
					}

				} else {
					//
					// Process the incoming data from a socket
					//
					processInput(fd, NULL);

					// not handling "keep alive" yet
					cleanup(fd);
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

	const int pollsize = 1;   // deprecated parameter, but must be > 0
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
