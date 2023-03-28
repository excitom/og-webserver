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
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "server.h"
#include "global.h"

/**
 * Process input from a socket
 */
void
processInput(int fd, SSL *ssl) {
	char *host = NULL;
	char *path = NULL;
	char *verb = NULL;
	char *queryString = NULL;
	char *p;
	char inbuff[BUFF_SIZE];
	char outbuff[BUFF_SIZE];

	size_t received;
	if (g.useTLS) {
		SSL_read_ex(ssl, (void *)&inbuff, BUFF_SIZE, &received);
	} else {
		received = recvData(fd, (char *)&inbuff, sizeof(inbuff));
	}

	snprintf(outbuff, BUFF_SIZE, "RECEIVED %d BYTES\n", received);
	doDebug(outbuff);
	if (received <= 0) {
		// no data to read
		doDebug("NO DATA\n");
		sendErrorResponse(fd, 400, "Bad Request", "No data");
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

		// find the query string, if any
		p = strchr(path, '?');
		if (p != NULL) {
			*p++ = '\0';
		}
		queryString = p;

		//
		// Only support the GET verb at this time
		//
		if (strcmp(verb, "GET") != 0) {
			sendErrorResponse(fd, 405, "Method Not Allowed", path);
			return;
		}
		handleGetVerb(fd, ssl, path, queryString);
	}

	return;
}
