#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include "server.h"
#include "global.h"

struct _fragment {
	struct _fragment *next;
	int len;
	char *fragment;
};
struct _fragment *fragments = NULL;
void addFragment(char *,char *);

/**
 * Show a listing of files in a directory 
 */
void
showDirectoryListing(int sockfd, SSL *ssl, char *path)
{
	DIR *dp;
	struct dirent *ep;
	char dirName[256];
	char *fullPath = (char *)&dirName;
	char buffer[1024];
	// the caller has already checked that strlen(docRoot+path) <= 255
	int size = strlen(path);
	if (path[size-1] != '/') {
		strcat(path, "/");
	}
	strcpy(fullPath, g.docRoot);
	strcat(fullPath, path);
	dp = opendir (fullPath);
	if (dp == NULL) {
		doDebug("Problem opening a directory, shouldn't happen");
		sendErrorResponse(sockfd, ssl, 404, "Not Found", path);
	}
	while ((ep = readdir(dp)) != NULL) {
		addFragment(path, ep->d_name);
	}
	closedir(dp);
	char *header =	"<html><head><title>Directory Listing</title></head>"
					"<body><h1>Directory Listing</h1><ul>";
	char *footer =	"</ul></body></html>";
	int contentLength = strlen(header) + strlen(footer);
	struct _fragment *f = fragments;
	while(f != NULL) {
		contentLength += f->len;
		f = f->next;
	}
	unsigned char ts[TIME_BUF];
	getTimestamp((unsigned char *)&ts, RESPONSE_FORMAT);
	int httpCode = 200;
	char *responseHeaders = 
"HTTP/1.1 %d OK\r\n"
"Server: ogws/0.1\r\n"
"Date: %s\r\n"
"Content-Type: text/html\r\n"
"Content-Length: %d\r\n\r\n";

	int sz = snprintf(buffer, BUFF_SIZE, responseHeaders, httpCode, ts, contentLength);
	// send the response headers
	sendData(sockfd, ssl, (char *)&buffer, sz);

	sendData(sockfd, ssl, header, strlen(header));
	while(fragments != NULL) {
		struct _fragment *f = fragments;
		fragments = f->next;
		sendData(sockfd, ssl, f->fragment, f->len);
		free(f->fragment);
		free(f);
	}
	sendData(sockfd, ssl, footer, strlen(footer));
	return;
}

/**
 * allocate and chain a new HTML fragment struct
 */
void
addFragment(char *dirPath, char *fileName)
{
	if (fileName[0] == '.' && fileName[1] == '\0') {
		return;		// skip dot entry
	}
	struct _fragment *f = malloc(sizeof(struct _fragment));
	f->next = NULL;
	if (fragments == NULL) {
		fragments = f;
	} else {
		struct _fragment *list = fragments;
		struct _fragment *prev;
		while(list != NULL) {
			prev = list;
			list = list->next;
		}
		prev->next = f;
	}
	//
	// fragment structure:
	// <li><a href="PATH">FILE NAME</a></li>
	//
	char buffer[BUFF_SIZE];
	f->len = snprintf(buffer, BUFF_SIZE, "<li><a href=\"%s%s\">%s</a></li>", dirPath, fileName, fileName);
	f->fragment = (char *)malloc(f->len+1);
	strcpy(f->fragment, (char *)&buffer);
	return;
}
