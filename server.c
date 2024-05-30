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
#include "serverlist.h"
#include "server.h"

char buff[BUFF_SIZE];
char* buffer = (char *)&buff;

void
server(int portNum, int errorFd)
{
	int epollFd = epollCreate();
	const int isTLS = 0;
	int sockFd = createBindAndListen(isTLS, portNum);
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.u64 = 0LL;
	ev.data.fd = sockFd;

	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, sockFd, &ev) < 0) {
		snprintf(buffer, BUFF_SIZE, "Couldn't add server socket %d to epoll set: %m\n", sockFd);
		doDebug(buffer);
		cleanup(sockFd);	  
		exit(1);
	}

	//
	// Main event loop
	//
	while(1) {
		doDebug("Starting epoll_wait");

		int rval;
		struct epoll_event epoll_events[getWorkerConnections()];
		//
		// Loop if interrupted by a signal
		//
		while ((rval = epoll_wait(epollFd, epoll_events, getWorkerConnections(), -1)) < 0) {
			if ((rval < 0) && (errno != EINTR)) {
				doDebug("epoll_wait failed");
				cleanup(sockFd);
				return;
			}
		}

		//
		// Loop over returned events
		//
		for (int i = 0; i < rval; i++) {
			int clientFd;	  // descriptor for client connection
			uint32_t events;	// events map
			events = epoll_events[i].events;
			int fd = epoll_events[i].data.fd;

			//
			// Misc error
			//
			if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
				if (fd == sockFd) {
					doDebug("epoll_wait failed");
					doDebug(buffer);
					cleanup(sockFd);
					exit(1);
				} else {
					snprintf(buffer, BUFF_SIZE, "Closing socket with sockFd %d\n", fd);
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
				if (fd == sockFd) {
					struct sockaddr_in peerAddr;
					socklen_t salen = sizeof(peerAddr);
					while ((clientFd = accept(sockFd, (struct sockaddr *) &peerAddr, &salen)) < 0) {
						if ((clientFd < 0) && (errno != EINTR)) {
							fprintf(stderr, "Accept on socket %d failed: %m\n", sockFd);
							cleanup(sockFd);
							return;
						} else {
							fprintf(stderr, "Resuming interrupted `accept()`\n");
						}
					}
					// The TLS server is multi-threaded, hence can have 
					// multiple client connections running concurrently.
					// The non-TLS server (currently) only has one connection
					// at a time. We queue the connection here, but don't need
					// to keep track of the returned object since there is
					// only one.
					queueClientConnection(clientFd, errorFd, peerAddr, NULL);

					//
					// Add a new event to listen for
					//
					ev.events = EPOLLIN;
					ev.data.u64 = 0LL;
					ev.data.fd = clientFd;	

					if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &ev) < 0) {
						snprintf(buffer, BUFF_SIZE, "Couldn't add client socket %d to epoll set: %m\n", clientFd);
						doDebug(buffer);
						cleanup(sockFd);	  
						exit(1);
					}

				} else {
					//
					// Process the incoming data from a socket
					//
					_request *req = (_request *)calloc(1,sizeof(_request));
					req->errorFd = errorFd;
					req->clientFd = fd;
					req->ssl = NULL;
					processInput(req);
					if (req->path) free(req->path);
					if (req->fullPath) free(req->fullPath);
					if (req->queryString) free(req->queryString);
					if (req->headers) free(req->headers);
					if (req->verb) free(req->verb);
					if (req->protocol) free(req->protocol);
					if (req->host) free(req->host);
					free(req);

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

	const int pollSize = 1;   // deprecated parameter, but must be > 0
	int epollFd = epoll_create(pollSize);

	if (epollFd < 0) {
		snprintf(buffer, BUFF_SIZE, "Could not create the epoll fd: %m");
		doDebug(buffer);
		exit(1);
	}
	snprintf(buffer, BUFF_SIZE, "epoll fd created successfully");
	doDebug(buffer);
	return epollFd;
}
