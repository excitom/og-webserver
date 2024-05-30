/**
 * Reverse-proxy a request to an upstream target using FastCGI protocol.
 *
 * (c) Tom Lang 11/2023
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
typedef struct _param {
	char *key;
	char *paramName;
	char *value;
}_param;

_param fastCgiParams[] = {
	{"QUERY_STRING", "$query_string", NULL},
	{"REQUEST_METHOD", "$request_method",  NULL},
	{"CONTENT_TYPE", "$content_type", NULL},
	{"CONTENT_LENGTH", "$content_length",  NULL},
	{"SCRIPT_FILENAME", "$document_root$fastcgi_script_name", NULL},
	{"SCRIPT_NAME", "$fastcgi_script_name", NULL},
	{"PATH_INFO", "$fastcgi_path_info", NULL},
	{"PATH_TRANSLATED", "$document_root$fastcgi_path_info", NULL},
	{"REQUEST_URI", "$request_uri", NULL},
	{"DOCUMENT_URI", "$document_uri", NULL},
	{"DOCUMENT_ROOT", "$document_root", NULL},
	{"SERVER_PROTOCOL", "$server_protocol", NULL},
	{"GATEWAY_INTERFACE", "CGI/1.1", NULL},
	{"SERVER_SOFTWARE", "ogws/$version", NULL},
	{"REMOTE_ADDR", "$remote_addr", NULL},
	{"REMOTE_PORT", "$remote_port", NULL},
	{"SERVER_ADDR", "$server_addr", NULL},
	{"SERVER_PORT", "$server_port", NULL},
	{"SERVER_NAME", "$server_name", NULL},
	{"HTTPS", "$https", NULL},
	{"REDIRECT_STATUS", "200", NULL}
};

_param *
linearSearch( _param* params, size_t size, const char *key)
{
	for (size_t i = 0; i < size; i++) {
		if (strcmp(params[i].key, key) == 0) {
			return &params[i];
		}
	}
	return NULL;
}

/**
 * The parameters are in a fixed list, and this function is just
 * checking that we're not trying to do something the fixed list
 * can't handle. Maybe someday this will be dynamic.
 */
void
checkParameter(char *key, char *value)
{
	size_t items = sizeof(fastCgiParams) / sizeof(_param);
	_param *param = linearSearch(fastCgiParams, items, key);
	if (param) {
		if (strcmp(param->paramName, value) != 0) {
			fprintf(stderr, "Attempt to change default fastcgi_param %s to %s, not supported, ignored\n", key, value);
		}
	} else {
		fprintf(stderr, "Unregognized fastcgi_param %s, ignored\n", key);
	}
	free(key);
	free(value);
}

/**
 * This is called for each request to a location that has `fastcgi_pass`
 * defined, and it initializes all the variables.
 */
void
initParameters(_request *req)
{
	size_t items = sizeof(fastCgiParams) / sizeof(_param);
	for (size_t i = 0; i < items; i++) {
		if (strcmp(fastCgiParams[i].key, "QUERY_STRING") == 0) {
			fastCgiParams[i].value = req->queryString;
		}
		else if (strcmp(fastCgiParams[i].key, "REQUEST_METHOD") == 0) {
			fastCgiParams[i].value = req->verb;
		}
		else if (strcmp(fastCgiParams[i].key, "CONTENT_TYPE") == 0) {
			// tbd
		}
		else if (strcmp(fastCgiParams[i].key, "CONTENT_LENGTH") == 0) {
			// tbd
		}
		else if (strcmp(fastCgiParams[i].key, "GATEWAY_INTERFACE") == 0) {
			if (!fastCgiParams[i].value) {
				fastCgiParams[i].value = fastCgiParams[i].paramName;
			}
		}
	}
}

void
handleFastCGIPass(_request *req)
{
	initParameters(req);
	int upstream = getUpstreamServer(req);
	if (upstream < 0) {
		doDebug("upstream failed");
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
		_clientConnection *c = getClient(req->clientFd);
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
	int httpCode = 200;
	int size = 0;
	do {
		bytes = recvData(upstream, buffer, BUFF_SIZE);
		if (bytes == 0) {
			break;
		}
		size += bytes;
		sendData(req->clientFd, req->ssl, (char *)&buffer, bytes);
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
	accessLog(req->clientFd, req->server->accessLog->fd, req->verb, httpCode, req->path, size);
	return;
}
