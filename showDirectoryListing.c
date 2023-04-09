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
	char *header =	"<html><head>"
					"<title>Directory Listing</title>"
					"<style type=\"text/css\">body {font-family: system-ui;}"
					".sz {font-size: 10px;}</style>"
					"</head>"
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

	// send the response body
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
	// Get the file size
	//
	char buffer[BUFF_SIZE];
	char *fullPath = (char *)&buffer;
	strcpy(fullPath, g.docRoot);
	strcat(fullPath, dirPath);
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
		f->len = snprintf(buffer, BUFF_SIZE, "<li><a href=\"%s%s\">%s</a> Directory</li>", dirPath, fileName, fileName, fileSize);
	} else {
		f->len = snprintf(buffer, BUFF_SIZE, "<li><a href=\"%s%s\">%s</a><span  class=\"sz\"> - %'d Bytes</span></li>", dirPath, fileName, fileName, fileSize);
	}
	f->fragment = (char *)malloc(f->len+1);
	strcpy(f->fragment, (char *)&buffer);
	return;
}
