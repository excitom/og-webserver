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

/**
 * Send an error response to a HTTP request
 */
void
sendErrorResponse( int fd, int code, char *msg )
{
	doDebug(msg);
	char buffer1[BUFF_SIZE];
	char *responseBody = 
"<html>\r\n"
"<head><title>%d %s</title></head>\r\n"
"<center><h1>%d %s</h1></center>\r\n"
"</body>\r\n"
"</html>\r\n";

	int sz1 = snprintf(buffer1, BUFF_SIZE, responseBody, code, msg, code, msg);

	char buffer2[64];
	int sz2 = snprintf(buffer2, 64, "Content-Length: %d\r\n\r\n", sz1);

	unsigned char ts[TIME_BUF];
	getTimestamp((unsigned char *)&ts, RESPONSE_FORMAT);

	char buffer3[BUFF_SIZE];
	char *responseHeaders = 
"HTTP/1.1 %d %s\r\n"
"Server: ogws/0.1\r\n"
"Date: %s\r\n"
"Content-Type: text/html\r\n";

	int sz3 = snprintf(buffer3, BUFF_SIZE, responseHeaders, code, msg, ts);

	sendData(fd, buffer3, sz3);
	sendData(fd, buffer2, sz2);
	sendData(fd, buffer1, sz1);
	shutdown(fd, SHUT_RDWR);
	close(fd);
	return;
}
