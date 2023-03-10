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
#include <sys/sendfile.h>
#include "server.h"
#include "global.h"

/**
 * Handle a GET request
 */
void
handleGetVerb(int sockfd, char *path)
{
	// todo: disallow ../ in the path
	char fullPath[256];
	char buffer[BUFF_SIZE];
	strcpy(fullPath, g.docRoot);
	strcat(fullPath, path);
	int fd = open(fullPath, O_RDONLY);
	if (fd == -1) {
		int e = errno;
		snprintf(buffer, BUFF_SIZE, "%s: file open failed: %s\n", fullPath, strerror(e));
		doDebug(buffer);
		sendErrorResponse(sockfd, 404, "Not Found");
	}
  	int size = lseek(fd, 0, SEEK_END);

	unsigned char ts[TIME_BUF];
	getTimestamp((unsigned char *)&ts, RESPONSE_FORMAT);

	char *responseHeaders = 
"HTTP/1.1 200 OK\r\n"
"Server: ogws/0.1\r\n"
"Date: %s\r\n"
"Content-Type: text/html\r\n"
"Content-Length: %d\r\n\r\n";

	int sz = snprintf(buffer, BUFF_SIZE, responseHeaders, ts, size);
	int sent = sendData(sockfd, buffer, sz);
	if (sent != sz) {
		doDebug("Problem sending response headers");
	}

	off_t offset;
	sent = sendfile(sockfd, fd, &offset, size);
	if (sent != size) {
		doDebug("Problem sending response body");
	}
	close(fd);
	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);
	return;
}
