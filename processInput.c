/**
 * Process input from a socket. The expected input is an 
 * HTTP request.
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
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "server.h"
#include "global.h"

_server *getServerForHost(char *);

void
processInput(int fd, SSL *ssl) {
	char *host = NULL;
	char *path = NULL;
	char *verb = NULL;
	char *queryString = NULL;
	char *p;
	char inbuff[BUFF_SIZE];
	char outbuff[BUFF_SIZE];
	char *input = (char *)&inbuff;
	size_t received;
	memset(input, 0, BUFF_SIZE);
	if (ssl) {
		SSL_read_ex(ssl, (void *)input, BUFF_SIZE, &received);
	} else {
		received = recvData(fd, input, BUFF_SIZE);
	}

	snprintf(outbuff, BUFF_SIZE, "RECEIVED %d BYTES\n", (int)received);
	doDebug(outbuff);
	if (received <= 0) {
		// no data to read
		doDebug("NO DATA\n");
		sendErrorResponse(fd, ssl, 400, "Bad Request", "No data");
		return;
	} else {
		// Save all the headers in case of `proxy_pass`
		char *headers = (char *)malloc(strlen(input)+1);
		strcpy(headers, input);
		// Expected format:
		// verb path HTTP/1.1\r\n
		// Host: nn.nn.nn.nn:pp\r\n
		// ... other headers we are ignoring for now
		verb = input;
		p = strchr(verb, ' ');
		if (!p) {
			doDebug("Bad data");
			sendErrorResponse(fd, ssl, 400, "Bad Request", "Bad data");
			free(headers);
			return;
		}
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
			sendErrorResponse(fd, ssl, 400, "Bad Request", path);
			free(headers);
			return;
		}

		// find the query string, if any
		p = strchr(path, '?');
		if (p != NULL) {
			*p++ = '\0';
			queryString = p;
		}
		_server *server = getServerForHost(host);
		_location *loc = getDocRoot(server, path);
		if (!loc) {
			doDebug("No doc root.");
			return;
		}

		//
		// check for proxy_pass
		//
		if (loc->type == TYPE_PROXY_PASS) {
			handleProxyPass(fd, headers, loc);
			free(headers);
			return;
		}
		free(headers);	// no longer need a copy of the headers
		char *docRoot = loc->target;

		//
		// Only support the GET verb at this time
		// (except for proxy_pass
		//
		if (strcmp(verb, "GET") != 0) {
			sendErrorResponse(fd, ssl, 405, "Method Not Allowed", path);
			free(headers);
			return;
		}

		// todo: disallow ../ in the path
		int size = strlen(docRoot) + strlen(path);
		const int maxLen = 255;
		if (size > maxLen) {
			doDebug("URI too long");
			char truncated[maxLen+5];
			strncpy(truncated, path, maxLen);
			truncated[maxLen] = '\0';
			strcat(truncated, "...");
			sendErrorResponse(fd, ssl, 414, "URI too long", truncated);
			return;
		}

		handleGetVerb(fd, ssl, server, docRoot, path, queryString);
	}
	return;
}

/**
 * Find the server information based on the hostname
 */
_server *
getServerForHost(char *host)
{
	char *p = strchr(host, ':');
	int port = g.defaultServer->port;
	if (p) {
		port = atoi(p+1);
		*p = '\0';
	} 
	_server *server = g.servers;
	size_t hostLen = strlen(host);
	while(server != NULL) {
		if ((hostLen == strlen(server->serverName)) 
				&& (strcmp(host, server->serverName) == 0)
				&& (server->port == port)) {
			break;
		}
		server = server->next;
	}
	if (server == NULL) {
		server = g.defaultServer;
	}
	return server;
}
