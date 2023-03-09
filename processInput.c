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
	char *port = NULL;
	char *path = NULL;
	char *verb = NULL;
	char *p;
	char inbuff[BUFF_SIZE];
	char outbuff[BUFF_SIZE];

	int received = recvData(fd, (char *)&inbuff, sizeof(inbuff));

	snprintf(outbuff, BUFF_SIZE, "RECEIVED %d BYTES\n", received);
	doDebug(outbuff);
	if (received <= 0) {
		// no data to read
		doDebug("NO DATA\n");
		shutdown(fd, SHUT_RDWR);
		close(fd);
		return;
	} else {
		// Expected format:
		// verb path HTTP/1.1\r\n
		// Host: nn.nn.nn.nn:pp\r\n
		// ... other headers we are ignoring
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
			doDebug("Bad request");
			sendErrorResponse(fd, 400, "Bad Request");
			shutdown(fd, SHUT_RDWR);
			close(fd);
			return;
		}
		printf("VERB: %s\nPATH: %s\nHOST %s\n", verb, path, host);
	}

	return;
}
