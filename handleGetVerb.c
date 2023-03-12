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
#include <sys/stat.h>
#include "server.h"
#include "global.h"

/**
 * Handle a GET request
 */
void
handleGetVerb(int sockfd, char *path)
{
	// todo: disallow ../ in the path
	int size = strlen(g.docRoot) + strlen(path);
	if (size > 255) {
		doDebug("URI too long");
		sendErrorResponse(sockfd, 414, "URI too long");
		return;
	}

	char fullPath[300];		// plenty of room to tack on index.html, if needed
	char buffer[BUFF_SIZE];
	strcpy(fullPath, g.docRoot);
	strcat(fullPath, path);

	struct stat sb;
	if (stat(fullPath, &sb) == -1) {
		int e = errno;
		snprintf(buffer, BUFF_SIZE, "%s: file stat failed: %s\n", fullPath, strerror(e));
		doDebug(buffer);
		sendErrorResponse(sockfd, 404, "Not Found");
		return;
	}

	// if the path pointing to a directory?
	if (sb.st_mode & S_IFMT == S_IFDIR) {
		if (fullPath[size-1] != '/') {
			strcat(fullPath, "/");
		}
		// note: not supporting directory listings
		strcat(fullPath, "index.html");
	}

	// guess the mime type by the extension, if any
	char mt[256];
	char *mimeType = (char *)&mt;
	getMimeType(path, mimeType);

	int fd = open(fullPath, O_RDONLY);
	if (fd == -1) {
		int e = errno;
		snprintf(buffer, BUFF_SIZE, "%s: file open failed: %s\n", fullPath, strerror(e));
		doDebug(buffer);
		sendErrorResponse(sockfd, 404, "Not Found");
		return;
	}

	// get the content length
	size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	unsigned char ts[TIME_BUF];
	getTimestamp((unsigned char *)&ts, RESPONSE_FORMAT);

	char *responseHeaders = 
"HTTP/1.1 200 OK\r\n"
"Server: ogws/0.1\r\n"
"Date: %s\r\n"
"Content-Type: %s\r\n"
"Content-Length: %d\r\n\r\n";

	int sz = snprintf(buffer, BUFF_SIZE, responseHeaders, ts, mimeType, size);
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

/**
 * Guess the mime type of a file using its extension, if any.
 * Default: html
 */
void
getMimeType(char *name, char *mimeType)
{
	strcpy(mimeType, "text/html");	// default
	char *p = strrchr(name, '.');
	if (p != NULL) {
		p++;
		struct _mimeTypes *mt = g.mimeTypes;
		while (mt != NULL) {
			if (strcmp(p, mt->extension) == 0) {
				strcpy(mimeType, mt->mimeType);
				break;;
			}
			mt = mt->next;
		}
	}
	return;
}

