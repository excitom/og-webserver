/**
 * Handle a HTTP GET or HEAD request
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
handleGetVerb(_request *req)
{
	if (req->queryString != NULL) {
		doDebug("Query string ignored, not yet implemented.");
	}
	if (pathExists(req, req->path) == -1) {
		sendErrorResponse(req, 404, "Not Found", req->path);
		return;
	}

	// if the path pointing to a directory?
	if (req->isDir) {
		// default to the index file if not specified
		req->localFd = openDefaultIndexFile(req);
		if (req->localFd == -1) {
			// if the path is a directory, and the index file is not present,
			// do we want to show a directory listing?
			if (req->server->autoIndex) {
				showDirectoryListing(req);
			} else {
				if (isDebug()) {
					fprintf(stderr, "%s: file open failed: %s\n", req->fullPath, strerror(errno));
				}
				sendErrorResponse(req, 404, "Not Found", req->path);
			}
			return;
		}
	} else {
		// not a directory
		req->localFd = open(req->fullPath, O_RDONLY);
		if (req->localFd == -1) {
			if (isDebug()) {
				fprintf(stderr, "%s: file open failed: %s\n", req->fullPath, strerror(errno));
			}
			sendErrorResponse(req, 404, "Not Found", req->path);
			return;
		}
	}

	serveFile(req);
	return;
}

/**
 * Server a file to a client
 */
void
serveFile(_request *req)
{
	// guess the mime type by the extension, if any
	char mt[256];
	char *mimeType = (char *)&mt;
	getMimeType(req->fullPath, mimeType);

	// get the content length
	size_t size = lseek(req->localFd, 0, SEEK_END);
	lseek(req->localFd, 0, SEEK_SET);

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
	size_t sz = snprintf(buffer, BUFF_SIZE, responseHeaders, httpCode, getVersion(), ts, mimeType, size);
	size_t sent = sendData(req->clientFd, req->ssl, buffer, sz);
	if (sent != sz) {
		doDebug("Problem sending response headers");
	}
	// only send the response body if the verb is GET
	if (strcmp(req->verb, "GET") == 0) {
		sendFile(req, size);
		accessLog(req->clientFd, req->server->accessLog->fd, "GET", httpCode, req->path, size);
	} else {
		accessLog(req->clientFd, req->server->accessLog->fd, "HEAD", httpCode, req->path, size);
	}
	close(req->localFd);
	return;
}

/**
 * Guess the mime type of a file using its extension, if any.
 * Default: html
 */
void
getMimeType(char *name, char *mimeType)
{
	strcpy(mimeType, g.defaultType);	// default
	char *p = strrchr(name, '.');
	if (p != NULL) {
		p++;
		struct _mimeTypes *mt = g.mimeTypes;
		while (mt != NULL) {
			if (strcmp(p, mt->extension) == 0) {
				strcpy(mimeType, mt->mimeType);
				break;
			}
			mt = mt->next;
		}
	}
	return;
}

/**
 * Open the default index file for a directory.
 * Return the file descriptor on success.
 */
int
openDefaultIndexFile(_request *req)
{
	char indexPath[300];
	_index_file *ifn = req->server->indexFiles;
	while(ifn) {
		strcpy(indexPath, req->fullPath);
		if (ifn->indexFile[0] != '/') {
			strcat(indexPath, "/");
		}
		strcat(indexPath, ifn->indexFile);
		int fd = open(indexPath, O_RDONLY);
		if (fd >= 0) {
			// update the full path to include the index file
			strcpy(req->fullPath, indexPath);
			return fd;
		}
		ifn = ifn->next;
	}
	return -1;
}

/**
 * Check if a path exists.
 * The fullPath parameter is updated.
 *
 * Note: Even though `request` has a `path` member, the input path may be
 * something different (see `try_files`) so it is passed explicitly.
 */
int
pathExists(_request *req, char *path)
{
	req->fullPath = (char *)calloc(1, 300);
	req->isDir = 0;
	strcpy(req->fullPath, req->loc->root);
	if (path[0] != '/') {
		strcat(req->fullPath, "/");
	}
	strcat(req->fullPath, path);
	struct stat sb;
	if (stat(req->fullPath, &sb) == -1) {
		int e = errno;
		if (isDebug()) {
			fprintf(stderr, "%s: file stat failed: %s\n", req->fullPath, strerror(e));
		}
		return -1;
	}
	req->isDir = (S_ISDIR(sb.st_mode)) ? 1 : 0;
	return 1;
}
