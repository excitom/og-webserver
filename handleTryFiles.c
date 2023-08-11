/**
 * Handle a `try_files` directive. We've already serverd the request if
 * there was a matching file or directory, so now check if there is a 
 * fallback file to use.
 *
 * Example directives:
 * 		location / {
 * 			try_files $uri $uri/ /index.php;
 * 		}
 *
 * 	In this example, if the file exists, serve it. If the path is a directory
 * 	with an index file, servve that index file. Otherwise send everything
 * 	to index.php.
 *
 * 	Note: This is the only `try_files` format implemented at this time.
 *
 * (c) Tom Lang 8/2023
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

static char *tryTarget;
static char *fullPath;

/**
 * If the requested path is not a file or directory, 
 * serve the default file.
 */
void
serveDefaultFile(_request *req)
{
	free(req->path);
	req->path = (char *)malloc(strlen(tryTarget)+1);
	strcpy(req->path, tryTarget);
	free(req->fullPath);
	fullPath = (char *)malloc(strlen(req->loc->root)+strlen(tryTarget)+1);
	strcpy(req->fullPath, req->loc->root);
	strcat(req->fullPath, tryTarget);
	req->fd = open(req->fullPath, O_RDONLY);
	if (req->fd == -1) {
		sendErrorResponse(req, 404, "Not Found", req->path);
	} else {
		serveFile(req);
	}
}

/**
 * At this time, only a specific format is implemented.
 * 	try_files $uri $uri/ /index_file
 */
char *
checkTryFilesFormat(_location *loc)
{
	_try_target *tt = loc->try_target;
	if (!tt) {
		return NULL;
	}
	if ((strlen(tt->target) != 4)
		|| (strcmp(tt->target, "$uri") != 0)) {
		return NULL;
	}
	tt = tt->next;
	if (!tt) {
		return NULL;
	}
	if ((strlen(tt->target) != 5)
		|| (strcmp(tt->target, "$uri/") != 0)) {
		return NULL;
	}
	tt = tt->next;
	if (!tt) {
		return NULL;
	}
	tryTarget = tt->target;
	return tt->target;
}

/**
 * Implement the `try_files` directive
 */
void
handleTryFiles(_request *req)
{
	char *tryTarget = checkTryFilesFormat(req->loc);
	if (!tryTarget) {
		fprintf(stderr, "Unrecognized `try_files` format\n");
		sendErrorResponse(req, 404, "Not Found", req->path);
	}
	req->isDir = 0;
	if (pathExists(req, req->path) == -1) {
		serveDefaultFile(req);
		return;
	}
	if (req->isDir == 0) {
		req->fd = open(req->fullPath, O_RDONLY);
		if (req->fd != -1) {
			serveDefaultFile(req);
			return;
		}
	}

	// this is a directory
	req->fd = openDefaultIndexFile(req);
	if (req->fd != -1) {
		serveDefaultFile(req);
		return;
	} else {
		// the index file is not present,
		// do we want to show a directory listing?
		if (req->server->autoIndex) {
			showDirectoryListing(req);
			return;
		}
	}

	// specific file or directory do not exist, fall back to the try target
	if (pathExists(req, tryTarget) == -1) {
		sendErrorResponse(req, 404, "Not Found", tryTarget);
		return;
	}
	if (req->isDir == 0) {
		req->fd = open(req->fullPath, O_RDONLY);
		if (req->fd != -1) {
			serveDefaultFile(req);
			return;
		}
	}
	sendErrorResponse(req, 404, "Not Found", req->path);
	
	return;
}
