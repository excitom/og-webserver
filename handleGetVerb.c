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
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include "server.h"
#include "global.h"

/**
 * Handle a GET request
 */
void
handleGetVerb(int sockfd, SSL *ssl, char *path, char *queryString)
{
	// todo: disallow ../ in the path
	int size = strlen(g.docRoot) + strlen(path);
	int maxLen = 255;
	if (size > maxLen) {
		doDebug("URI too long");
		char truncated[maxLen+5];
		strncpy(truncated, path, maxLen);
		truncated[maxLen] = '\0';
		strcat(truncated, "...");
		sendErrorResponse(sockfd, ssl, 414, "URI too long", truncated);
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
		sendErrorResponse(sockfd, ssl, 404, "Not Found", path);
		return;
	}

	// if the path pointing to a directory?
	if (S_ISDIR(sb.st_mode)) {
		if (fullPath[size-1] != '/') {
			strcat(fullPath, "/");
		}
		// default to the index file if not specified
		strcat(fullPath, "index.html");
	}

	// guess the mime type by the extension, if any
	char mt[256];
	char *mimeType = (char *)&mt;
	getMimeType(path, mimeType);

	int fd = open(fullPath, O_RDONLY);
	if (fd == -1) {
		// if the path is a directory, and the index file is not present,
		// do we want to show a directory listing?
		if (S_ISDIR(sb.st_mode) && g.dirList) {
			showDirectoryListing(sockfd, ssl, path);
		} else {
			snprintf(buffer, BUFF_SIZE, "%s: file open failed: %s\n", fullPath, strerror(errno));
			doDebug(buffer);
			sendErrorResponse(sockfd, ssl, 404, "Not Found", path);
		}
		return;
	}

	// get the content length
	size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	unsigned char ts[TIME_BUF];
	getTimestamp((unsigned char *)&ts, RESPONSE_FORMAT);

	int httpCode = 200;
	char *responseHeaders = 
"HTTP/1.1 %d OK\r\n"
"Server: ogws/0.1\r\n"
"Date: %s\r\n"
"Content-Type: %s\r\n"
"Content-Length: %d\r\n\r\n";

	int sz = snprintf(buffer, BUFF_SIZE, responseHeaders, httpCode, ts, mimeType, size);
	size_t sent = sendData(sockfd, ssl, buffer, sz);
	if (sent != sz) {
		doDebug("Problem sending response headers");
	}

	off_t offset = 0;
	if (g.useTLS) {
		//sent = SSL_sendfile(ssl, fd, offset, size, 0);
		char *p = malloc(size);
		read(fd, p, size);
		if (SSL_write_ex(ssl, p, size, &sent) == 0) {
			if (g.debug) {
				ERR_print_errors_fp(stderr);
			}
		}
		free(p);
	} else {
		sent = sendfile(sockfd, fd, &offset, size);
	}
	if (sent != size) {
		snprintf(buffer, BUFF_SIZE, "Problem sending response body: SIZE %d SENT %d\n", size, sent);
		doDebug(buffer);
	}

	accessLog(sockfd, "GET", httpCode, path, size);
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
