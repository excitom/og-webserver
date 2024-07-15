/**
 * Send an HTTP error response to a bad HTTP request
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
#include "serverlist.h"
#include "server.h"

void
sendErrorResponse( _request *req, int code, char *msg, char *path )
{
	doDebug(msg);
	char buffer1[BUFF_SIZE];
	const char *responseBody = 
"<html>\r\n"
"<head><title>%d %s</title></head>\r\n"
"<center><h1>%d %s</h1></center>\r\n"
"</body>\r\n"
"</html>\r\n";

	int sz1 = snprintf(buffer1, BUFF_SIZE, responseBody, code, msg, code, msg);

	char buffer2[64];
	int sz2 = snprintf(buffer2, 64, "Content-Length: %d\r\n\r\n", sz1);

	char ts[TIME_BUF];
	getTimestamp((char *)&ts, RESPONSE_FORMAT);

	char buffer3[BUFF_SIZE];
	const char *responseHeaders = 
"HTTP/1.1 %d %s\r\n"
"Server: ogws/%s\r\n"
"Date: %s\r\n"
"Content-Type: text/html\r\n";

	int sz3 = snprintf(buffer3, BUFF_SIZE, responseHeaders, code, msg, getVersion(), ts);

	sendData(req->clientFd, req->ssl, buffer3, sz3);
	sendData(req->clientFd, req->ssl, buffer2, sz2);
	sendData(req->clientFd, req->ssl, buffer1, sz1);

	errorLog(req->clientFd, req->server, req->verb, code, path, msg);
	return;
}
