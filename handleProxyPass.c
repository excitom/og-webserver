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
#include "serverlist.h"
#include "server.h"
#include "global.h"

void
handleProxyPass(_request *req)
{
    struct sockaddr_in server;
    server.sin_family      = req->loc->passTo->sin_family;
    server.sin_port        = req->loc->passTo->sin_port;
    server.sin_addr.s_addr = req->loc->passTo->sin_addr.s_addr;
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
		close(upstream);
		return;
    }
	// change the \r\n\r\n which marks the end of the headers to
	// \r\n in order to add the `X-Forwarded-For` header
	char *r = strstr(req->headers, "\r\n\r\n");
	char *body = NULL;
	if (r) {
		r += 2;
		*r = '\0';
		body = r + 2;
	}
	sendData(upstream, NULL, req->headers, strlen(req->headers));
	char buffer[BUFF_SIZE];
	if (r) {
		strcpy((char *)&buffer, "X-Forwarded-For: ");
		_clientConnection *c = getClient(req->sockFd);
		strcat((char *)&buffer, c->ip);
		strcat((char *)&buffer, "\r\n\r\n");
		sendData(upstream, NULL, buffer, strlen(buffer));
	}
	if (body && strlen(body)) {
		sendData(upstream, NULL, body, strlen(body));
	}
	//
	// receive the upstream's response and
	// forward it to the client
	//
	size_t bytes = BUFF_SIZE;
	int startOfResponse = 1;
	int httpCode;
	int size = 0;
	do {
		bytes = recvData(upstream, buffer, BUFF_SIZE);
		if (bytes == 0) {
			break;
		}
		size += bytes;
		sendData(req->sockFd, NULL, (char *)&buffer, bytes);
		if (startOfResponse) {
			startOfResponse = 0;
			// extract the HTTP responsse code
			char *p = (char *)&buffer;
			p = strchr(p, ' ');
			char *q = ++p;
			p = strchr(q, ' ');
			*p = '\0';
			httpCode = atoi(q);
		}
	} while(bytes == BUFF_SIZE);
	shutdown(upstream, SHUT_RDWR);
	close(upstream);
	accessLog(req->sockFd, req->server->accessLog->fd, req->verb, httpCode, req->path, size);
	return;
}
