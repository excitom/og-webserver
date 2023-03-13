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
#include "server.h"
#include "global.h"

/**
 * Process input from a socket
 */
void
processInput(int fd)
{
	char *host = NULL;
	char *path = NULL;
	char *verb = NULL;
	char *protocol = "http";
	char *port = "80";
	char *p;
	char inbuff[BUFF_SIZE];
	char outbuff[BUFF_SIZE];

	int received = recvData(fd, (char *)&inbuff, sizeof(inbuff));

	snprintf(outbuff, BUFF_SIZE, "RECEIVED %d BYTES\n", received);
	doDebug(outbuff);
	if (received <= 0) {
		// no data to read
		doDebug("NO DATA\n");
		return;
	} else {
		// Expected format:
		// verb path HTTP/1.1\r\n
		// Host: nn.nn.nn.nn:pp\r\n
		// ... other headers we are ignoring for now
		//
		verb = (char *)&inbuff;
		p = strchr(verb, ' ');
		*p++ = '\0';
		path = p;
		p = strchr(path, ' ');
		*p++ = '\0';
		while(*p++) {
			if (strncmp(p, "Host: ", 6) == 0) {
				break;
			}
		}
		if (p) {
			host = p + 6;
			p = strchr(host, '\r');
			*p = '\0';
		}
		if (!verb || !path || ! host) {
			if (!path) {
				path = "/";
			}
			sendErrorResponse(fd, 400, "Bad Request", path);
			return;
		}

		// looking for "://" to separate host and protocol
		char *end = host + strlen(host) - 1;
		p = strchr(host, ':');
		if ((p != NULL) &&
			(p < end-3) &&
			(p[1] == '/') &&
			(p[2] == '/'))
		{
			// truncate the host to just be the protocol part
			*p = '\0';
			// only accepting HTTP at this point
			if (strncmp(host, "http", 4) != 0) {
				sendErrorResponse(fd, 406, "Not Acceptable", path);
				return;
			}
			protocol = host;
			host = p+1;
		}

		// looking for : to separate host and port
		p = strchr(host, ':');
		if (p != NULL) {
			*p = '\0';
			port = p+1;
		}

		//
		// Only support the GET verb at this time
		//
		if (strcmp(verb, "GET") != 0) {
			sendErrorResponse(fd, 405, "Method Not Allowed", path);
			return;
		}
		handleGetVerb(fd, path);
	}

	return;
}
