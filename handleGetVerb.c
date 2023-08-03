/**
 * Handle a HTTP GET request
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
#include <sys/stat.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include "serverlist.h"
#include "server.h"
#include "global.h"

void
handleGetVerb(int sockfd, SSL *ssl, _server *server, _location *loc, char *path, char *queryString)
{
	if (queryString != NULL) {
		doDebug("Query string ignored, not yet implemented.");
	}
	char fullPath[300];
	strcpy(fullPath, loc->root);
	strcat(fullPath, path);
	struct stat sb;
	if (stat(fullPath, &sb) == -1) {
		int e = errno;
		if (g.debug) {
			fprintf(stderr, "%s: file stat failed: %s\n", fullPath, strerror(e));
		}
		sendErrorResponse(sockfd, ssl, 404, "Not Found", path);
		return;
	}

	// if the path pointing to a directory?
	int fd;
	if (S_ISDIR(sb.st_mode)) {
		if (fullPath[strlen(fullPath)-1] != '/') {
			strcat(fullPath, "/");
		}
		// default to the index file if not specified
		fd = openDefaultIndexFile(server, fullPath);
		if (fd == -1) {
			// if the path is a directory, and the index file is not present,
			// do we want to show a directory listing?
			if (loc->autoIndex) {
				showDirectoryListing(sockfd, ssl, loc->root, path);
			} else {
				if (g.debug) {
					fprintf(stderr, "%s: file open failed: %s\n", fullPath, strerror(errno));
				}
				sendErrorResponse(sockfd, ssl, 404, "Not Found", path);
			}
			return;
		}
	} else {
		// not a directory
		fd = open(fullPath, O_RDONLY);
		if (fd == -1) {
			if (g.debug) {
				fprintf(stderr, "%s: file open failed: %s\n", fullPath, strerror(errno));
			}
			sendErrorResponse(sockfd, ssl, 404, "Not Found", path);
			return;
		}
	}

	// guess the mime type by the extension, if any
	char mt[256];
	char *mimeType = (char *)&mt;
	getMimeType(fullPath, mimeType);

	// get the content length
	size_t size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	char ts[TIME_BUF];
	getTimestamp((char *)&ts, RESPONSE_FORMAT);

	int httpCode = 200;
	char *responseHeaders = 
"HTTP/1.1 %d OK\r\n"
"Server: ogws/%s\r\n"
"Date: %s\r\n"
"Content-Type: %s\r\n"
"Content-Length: %d\r\n\r\n";

	char buffer[BUFF_SIZE];
	size_t sz = snprintf(buffer, BUFF_SIZE, responseHeaders, httpCode, g.version, ts, mimeType, size);
	size_t sent = sendData(sockfd, ssl, buffer, sz);
	if (sent != sz) {
		doDebug("Problem sending response headers");
	}
	sendFile(sockfd, fd, ssl, size);
	accessLog(sockfd, server->accessFd, "GET", httpCode, path, size);
	close(fd);
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

/**
 * open the default index file for a directory
 */
int
openDefaultIndexFile(_server *server, char *fullPath) {
	char indexPath[300];
	_server_name *sn = server->serverNames;
	while(sn) {
		strcpy(indexPath, fullPath);
		strcat(indexPath, server->indexFiles->indexFile);
		int fd = open(indexPath, O_RDONLY);
		if (fd >= 0) {
			return fd;
		}
		sn = sn->next;
	}
	return -1;
}
