/**
 * Reverse-proxy a request to an upstream target.
 *
 * (c) Tom Lang 4/2023
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
#include <netdb.h>
#include "server.h"
#include "global.h"

void
handleProxyPass(int fd, char *headers, _location *loc)
{
    struct sockaddr_in server;
    server.sin_family      = loc->passTo->sin_family;
    server.sin_port        = loc->passTo->sin_port;
    server.sin_addr.s_addr = loc->passTo->sin_addr.s_addr;
	int upstream;
    if ((upstream = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
		doDebug("upstream socket failed.");
		doDebug(strerror(errno));
		return;
    }
    if (connect(upstream, (struct sockaddr *)&server, sizeof(server)) < 0) {
		doDebug("upstream connect failed.");
		doDebug(strerror(errno));
		return;
    }
	// change trailing \r\n\r\n to \r\n in order to
	// add the `X-Forwarded-For` header
	char *r = strrchr(headers, '\r');
	*r = '\0';
	sendData(upstream, NULL, headers, strlen(headers));
	char buffer[BUFF_SIZE];
	strcpy((char *)&buffer, "X-Forwarded-For: ");
	_clientConnection *c = getClient(fd);
	strcat((char *)&buffer, c->ip);
	strcat((char *)&buffer, "\r\n\r\n");
	sendData(upstream, NULL, buffer, strlen(buffer));
	size_t bytes = BUFF_SIZE;
	do {
		bytes = recvData(upstream, (char *)&buffer, bytes);
		sendData(fd, NULL, (char *)&buffer, bytes);
		if (bytes == 0) {
			break;
		}
	} while(bytes == BUFF_SIZE);
	return;
}
