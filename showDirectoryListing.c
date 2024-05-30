/**
 * When a URL is requested which is a directory name and the 
 * directory does not contain an INDEX file, show a simple HTML
 * page which contains a listing of the files in the directory.
 * Each file name is a clickable link so that the file contents
 * can be displayed.
 *
 * This behavior can be considered a security flaw since it can reveal
 * information that was not inteded. Therefore it is optional to
 * show instead a 404 error.
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
#include <dirent.h>
#include <sys/stat.h>
#include "serverlist.h"
#include "server.h"

typedef struct _fragment {
	struct _fragment *next;
	int len;
	char *fragment;
} _fragment;
_fragment *fragments = NULL;
void addFragment(char *,char *,char *);

/**
 * Show a listing of files in a directory 
 */
void
showDirectoryListing(_request *req)
{
	DIR *dp;
	struct dirent *ep;
	char buffer[BUFF_SIZE];
	dp = opendir (req->fullPath);
	if (dp == NULL) {
		doDebug("Problem opening a directory, shouldn't happen");
		sendErrorResponse(req, 404, "Not Found", req->path);
		return;
	}
	while ((ep = readdir(dp)) != NULL) {
		addFragment(req->fullPath, req->path, ep->d_name);
	}
	closedir(dp);
	const char *header ="<html><head>"
						"<title>Directory Listing</title>"
						"<style type=\"text/css\">body {font-family: system-ui;}"
						".sz {font-size: 10px;}</style>"
						"</head>"
						"<body><h1>Directory Listing</h1><ul>";
	const char *footer ="</ul></body></html>";
	int contentLength = strlen(header) + strlen(footer);
	_fragment *f = fragments;
	while(f != NULL) {
		contentLength += f->len;
		f = f->next;
	}
	char ts[TIME_BUF];
	getTimestamp((char *)&ts, RESPONSE_FORMAT);
	int httpCode = 200;
	const char *responseHeaders = 
"HTTP/1.1 %d OK\r\n"
"Server: ogws/0.1\r\n"
"Date: %s\r\n"
"Content-Type: text/html\r\n"
"Content-Length: %d\r\n\r\n";

	int sz = snprintf(buffer, BUFF_SIZE, responseHeaders, httpCode, ts, contentLength);
	// send the response headers
	sendData(req->clientFd, req->ssl, (char *)&buffer, sz);

	// send the response body
	sendData(req->clientFd, req->ssl, header, strlen(header));
	while(fragments != NULL) {
		_fragment *f = fragments;
		fragments = f->next;
		sendData(req->clientFd, req->ssl, f->fragment, f->len);
		free(f->fragment);
		free(f);
	}
	sendData(req->clientFd, req->ssl, footer, strlen(footer));
	accessLog(req->clientFd, req->server->accessLog->fd, "GET", 200, req->path, contentLength);
	return;
}

/**
 * allocate and chain a new HTML fragment struct
 */
void
addFragment(char *dirPath, char *path, char *fileName)
{
	if (fileName[0] == '.' && fileName[1] == '\0') {
		return;		// skip dot entry
	}
	_fragment *f = malloc(sizeof(_fragment));
	f->next = NULL;
	if (fragments == NULL) {
		fragments = f;
	} else {
		_fragment *list = fragments;
		_fragment *prev;
		while(list != NULL) {
			prev = list;
			list = list->next;
		}
		prev->next = f;
	}
	//
	// Get the file size
	//
	char buffer[BUFF_SIZE];
	char *fullPath = (char *)&buffer;
	strcpy(fullPath, dirPath);
	strcat(fullPath, "/");
	strcat(fullPath, fileName);
	struct stat sb;
	int fileSize = 0;
	if (stat(fullPath, &sb) == 0) {
		fileSize = (int)sb.st_size;
	}
	
	//
	// fragment structure:
	// <li><a href="PATH">FILE NAME</a> SIZE Bytes</li>
	//
	if (S_ISDIR(sb.st_mode)) {
		f->len = snprintf(buffer, BUFF_SIZE, "<li><a href=\"%s/%s\">%s</a> Directory</li>", path, fileName, fileName);
	} else {
		f->len = snprintf(buffer, BUFF_SIZE, "<li><a href=\"%s/%s\">%s</a><span  class=\"sz\"> - %'d Bytes</span></li>", path, fileName, fileName, fileSize);
	}
	f->fragment = (char *)malloc(f->len+1);
	strcpy(f->fragment, (char *)&buffer);
	return;
}
