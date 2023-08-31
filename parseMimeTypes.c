/**
 * Parse the mime.types file.
 *
 * Assumption: The types listed in the file are ordered by frequency of 
 * use, roughly, so storing them in a list that is sequentially searched
 * is simple and sufficiently fast.
 *
 * Expected format: 
 * 	types {
 * 	  mimeType			extension;
 * 	  mimeType			extension extension;
 * 	  mimeType			extension;
 * 	  ....
 * 	  mimeType			extension extension extension;
 * 	}
 * 	The white space is ignored except for one space between mimeType and
 * 	extension. The whole file could be one long line.
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
#include "serverlist.h"
#include "server.h"
#include "global.h"

char *parseLine(char *);
void saveMimeType(char *, char *);
_mimeTypes *addMimeTypeEntry();

void
parseMimeTypes()
{
	g.mimeTypes = NULL;		// start with an empty list
	const char mimeTypeFile[] = "/mime.types";
	char *fileName = malloc(strlen(g.configFile)+strlen(mimeTypeFile)+1);
	strcpy(fileName, g.configFile);
	char *p = strrchr(fileName, '/');
	*p = '\0';
	strcat(fileName, mimeTypeFile);
	int fd = open(fileName, O_RDONLY);
	if (fd == -1) {
		int e = errno;
		char buffer[BUFF_SIZE];
		snprintf(buffer, BUFF_SIZE, "%s: file open failed: %s\n", fileName, strerror(e));
		doDebug(buffer);
		exit(1);
	}

	// allocate space to hold the whole file
  	int size = lseek(fd, 0, SEEK_END);
	char *data = malloc(size+1);
	p = data;
	if (p == NULL) {
		perror("Out of memory");
		exit(1);
	}
	memset(p, 0, size+1);
	lseek(fd, 0, SEEK_SET);
	int sz = read(fd, p, size);
	if (size != sz) {
		fprintf(stderr, "Can't read mime types: %d != %d\n", size, sz);
		exit(1);
	}
	if (g.debug) {
		fprintf(stderr, "MIME Type size: %d\n", size);
	}

	char *q = strchr(p, '{');
	if (q == NULL) {
		perror("Invalid mime.type file, no starting delimiter.");
		exit(1);
	}
	p = q+1;
	while(*p != '}') {
		while(*p == ' ' || *p == '\n') {
			p++;
		}
		if (*p == '\0') {
			perror("Invalid mime.type file, no ending delimiter.");
			exit(1);
		}
		if (*p != '}') {
			// a line ends with a ';'. Newline characters don't matter.
			p = parseLine(p);
		}
	}

	//if (g.debug) {
		//fprintf(stderr, "MIME Types:\n");
		//_mimeTypes *mt = g.mimeTypes;
		//while (mt != NULL) {
			//fprintf(stderr, "MIME Type: %s -- Extension: %s\n", mt->mimeType, mt->extension);
			//mt = mt->next;
		//}
	//}

	// free up the space for the file and file name
	free(data);
	free(fileName);
}

/**
 * Parse a line from the mime.types file
 *
 * Each line ends with a ';' and contains at least one space 
 * separating the mime type and the extension.
 *
 * The can be multiple extensions which are separated by space. They all
 * share the same mime type.
 */
char *
parseLine(char *p)
{
	char *q = strchr(p, ' ');
	if (q == NULL) {
		perror("Invalid mime.type file, malformed mime type.");
		exit(1);
	}
	*q = '\0';
	char *mimeType = p;		// save string for later

	// save the extension(s)
	p = q+1;
	while(*p == ' ') {
		p++;
	}
	if (*p == '\0') {
		perror("Invalid mime.type file, missing extension.");
		exit(1);
	}
	q = strchr(p, ';');
	if (q == NULL) {
		perror("Invalid mime.type file, missing line end delimiter.");
		exit(1);
	}
	*q = '\0';
	char *endOfLine = q+1;
	while((q = strchr(p, ' ')) != NULL) {
		*q = '\0';
		saveMimeType(p, mimeType);
		p = q+1;
	}
	saveMimeType(p, mimeType);
	
	return(endOfLine);
}

/**
 * Save an individual mime type in the list
 */
void
saveMimeType(char *extension, char *mimeType)
{
	_mimeTypes *mt = addMimeTypeEntry();

	// save the mime type
	char *buff = malloc(strlen(mimeType)+1);
	strcpy(buff, mimeType);
	mt->mimeType = buff;

	// save the extension
	buff = malloc(strlen(extension)+1);
	strcpy(buff, extension);
	mt->extension = buff;
	return;
}

/**
 * allocate and chain a new mime type struct
 */
_mimeTypes *
addMimeTypeEntry()
{
	_mimeTypes *mt = malloc(sizeof(_mimeTypes));
	mt->next = NULL;
	if (g.mimeTypes == NULL) {
		g.mimeTypes = mt;
	} else {
		_mimeTypes *list = g.mimeTypes;
		_mimeTypes *prev;
		while(list != NULL) {
			prev = list;
			list = list->next;
		}
		prev->next = mt;
	}
	return mt;
}
