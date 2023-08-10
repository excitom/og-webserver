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
#include "serverlist.h"
#include "server.h"
#include "global.h"

_server *getServerForHost(char *);

void
processInput(_request *req) {
	req->path = NULL;
	req->queryString = NULL;
	char *host = NULL;
	char *verb = NULL;
	char *path = NULL;
	char *protocol = NULL;
	char *p;
	char inbuff[BUFF_SIZE];
	char outbuff[BUFF_SIZE];
	char *input = (char *)&inbuff;
	size_t received;
	memset(input, 0, BUFF_SIZE);
	if (req->ssl) {
		SSL_read_ex(req->ssl, (void *)input, BUFF_SIZE, &received);
	} else {
		received = recvData(req->sockFd, input, BUFF_SIZE);
	}

	snprintf(outbuff, BUFF_SIZE, "RECEIVED %d BYTES\n", (int)received);
	doDebug(outbuff);
	if (received <= 0) {
		// no data to read
		doDebug("NO DATA\n");
		sendErrorResponse(req, 400, "Bad Request", "No data");
		return;
	} else {
		// Save all the headers in case of `proxy_pass`
		req->headers = (char *)calloc(1, strlen(input)+1);
		strcpy(req->headers, input);
		// Expected format:
		// verb path HTTP/1.1\r\n
		// Host: nn.nn.nn.nn:pp\r\n
		// ... other headers we are ignoring for now
		verb = input;
		p = strchr(verb, ' ');
		if (!p) {
			doDebug("Bad data");
			sendErrorResponse(req, 400, "Bad Request", "Bad data");
			return;
		}
		*p++ = '\0';
		req->verb = (char *)calloc(1, strlen(verb)+1);
		strcpy(req->verb, verb);
		path = p;
		p = strchr(path, ' ');
		*p++ = '\0';
		req->path = (char *)calloc(1, strlen(path)+1);
		strcpy(req->path, path);
		protocol = p;
		p = strchr(protocol, '\r');
		*p++ = '\0';
		req->protocol = (char *)calloc(1, strlen(protocol)+1);
		strcpy(req->protocol, protocol);
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
		req->host = (char *)calloc(1, strlen(host)+1);
		strcpy(req->host, host);
		if (!req->verb || !req->path || ! req->host) {
			if (!req->path) {
				req->path = "/";
			}
			sendErrorResponse(req, 400, "Bad Request", req->path);
			return;
		}

		// find the query string, if any
		p = strchr(req->path, '?');
		if (p != NULL) {
			*p++ = '\0';
			req->queryString = p;
		}
		req->server = getServerForHost(host);
		req->loc = getDocRoot(req->server, req->path);
		if (!req->loc || !req->loc->root) {
			doDebug("No doc root.");
			return;
		}

		// todo: disallow ../ in the path
		int size = strlen(req->loc->root) + strlen(req->path);
		const int maxLen = 255;
		if (size > maxLen) {
			doDebug("URI too long");
			char truncated[maxLen+5];
			strncpy(truncated, req->path, maxLen);
			truncated[maxLen] = '\0';
			strcat(truncated, "...");
			sendErrorResponse(req, 414, "URI too long", truncated);
			return;
		}

		//
		// check for proxy_pass
		//
		if (req->loc->type == TYPE_PROXY_PASS) {
			handleProxyPass(req);
			return;
		}


		// check for try_files
		if (req->loc->type == TYPE_TRY_FILES) {
			handleTryFiles(req);
			return;
		}

		//
		// Only support the GET verb at this time
		// (except for proxy_pass)
		//
		if (strcmp(verb, "GET") != 0) {
			sendErrorResponse(req, 405, "Method Not Allowed", verb);
			return;
		}

		handleGetVerb(req);
	}
	return;
}

/**
 * Find the server information based on the hostname
 */
int
hasPort(int portNum, _server *server)
{
	_port *p = server->ports;
	while(p) {
		if (p->portNum == portNum) {
			return 1;
		}
		p = p->next;
	}
	return 0;
}

_server *
getServerForHost(char *host)
{
	_server *server = g.servers;	// default server
	char *p = strchr(host, ':');
	int portNum = server->ports->portNum;
	if (p) {
		portNum = atoi(p+1);
		*p = '\0';
	} 
	size_t hostLen = strlen(host);
	while(server != NULL) {
		_server_name *sn = server->serverNames;
		while(sn != NULL) {
			if ((hostLen == strlen(sn->serverName)) 
					&& (strcmp(host, sn->serverName) == 0)
					&& (hasPort(portNum, server))) {
				return server;
			}
			sn = sn->next;
		}
		server = server->next;
	}
	// no explicit matches, use the default
	return g.servers;
}
