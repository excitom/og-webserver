#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include "server.h"
#include "global.h"

char *parseLine(char *);
void saveMimeType(char *, char *);
struct _mimeTypes *addMimeTypeEntry();

/**
 * Parse the mime.types file.
 *
 * Assumption: The types listed in the file are ordered by frequency of 
 * use, roughly, so storing them in a list that is sequentially searched
 * is simple and sufficiently fast.
 */
void
parseMimeTypes() {
	g.mimeTypes = NULL;		// start with an empty list
	char buffer[BUFF_SIZE];
	char *p = buffer;
	strcpy(p, g.configPath);
	strcat(p, "/mime.types");
	int fd = open(p, O_RDONLY);
	if (fd == -1) {
		int e = errno;
		snprintf(buffer, BUFF_SIZE, "%s: file open failed: %s\n", p, strerror(e));
		doDebug(buffer);
		exit(1);
	}
  	int size = lseek(fd, 0, SEEK_END);
	p = malloc(size+1);
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
			p = parseLine(p);
		}
	}

	if (g.debug) {
		fprintf(stderr, "MIME Types:\n");
		struct _mimeTypes *mt = g.mimeTypes;
		while (mt != NULL) {
			fprintf(stderr, "MIME Type: %s -- Extension: %s\n", mt->mimeType, mt->extension);
			mt = mt->next;
		}
	}
}

/**
 * Parse a line from the mime.types file
 */
char *
parseLine(char *p) {
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
	struct _mimeTypes *mt = addMimeTypeEntry();

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
struct _mimeTypes *
addMimeTypeEntry()
{
	struct _mimeTypes *mt = malloc(sizeof(struct _mimeTypes));
	mt->next = NULL;
	if (g.mimeTypes == NULL) {
		g.mimeTypes = mt;
	} else {
		struct _mimeTypes *list = g.mimeTypes;
		struct _mimeTypes *prev;
		while(list != NULL) {
			prev = list;
			list = list->next;
		}
		prev->next = mt;
	}
	return mt;
}
