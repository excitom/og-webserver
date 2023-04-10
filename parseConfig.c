/**
 * Parse the config file.
 *
 * The file format matches the nginx config file format.
 *
 * (c) Tom Lang 4/2023
 */
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

typedef char * (*processKeyword)(char *);
typedef struct _keywords {
	char *keyword;
	int len;
	int section;
}_keywords;

_keywords keywords[] = {
	{"pid", 3, 0},
	{"user", 4, 0},
	{"http", 4, 1},
	{"root", 4, 0},
	{"index", 5, 0},
	{"events", 6, 1},
	{"server", 6, 1},
	{"listen", 6, 0},
	{"location", 8, 1},
	{"sendfile", 8, 0},
	{"error_log", 9, 0},
	{"error_page", 10, 0},
	{"log_format", 10, 0},
	{"access_log", 10, 0},
	{"server_name", 11, 0},
	{"default_type", 12, 0},
	{"ssl_certificate", 15, 0},
	{"keepalive_timeout", 17, 0},
	{"worker_connections", 18, 0},
	{"ssl_certificate_key", 19, 0}
};

typedef struct _token {
	char *p;
	char *q;
}_token;

void removeComments(char *);
_token getToken(char *);
char * lookupKeyword(char *, char *);

void
parseConfig() {
	char *fileName = malloc(256);
	strcpy(fileName, g.configPath);
	strcat(fileName, "/ogws.conf");
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
	char *p = data;
	if (p == NULL) {
		perror("Out of memory");
		exit(1);
	}
	memset(p, 0, size+1);
	lseek(fd, 0, SEEK_SET);
	int sz = read(fd, p, size);
	if (size != sz) {
		fprintf(stderr, "Can't read config file: %d != %d\n", size, sz);
		exit(1);
	}
	if (g.debug) {
		fprintf(stderr, "Config file size: %d\n", size);
	}
	removeComments(p);
	while (*p) {
		_token token = getToken(p);
		p = token.p;
		char *keyword = token.q;
		printf("TOKEN %s\n", keyword);
		p = lookupKeyword(keyword, p);
	}

	// free up the space for the file and file name
	free(data);
	free(fileName);
}

/**
 * Remove comments lines so that they are ignored. Comments are
 * removed by changing them to blank space.
 * A comment starts with a '#' and ends with a newline character.
 */
void
removeComments(char *p)
{
	while(*p) {
		if (*p == '#') {
			while(*p && *p != '\n') {
				*p++ = ' ';
			}
		} else {
			p++;
		}
	}
}

/**
 * Get a token from the config file. A token is a string of characters
 * terminated by a space, tab, semi-colon, or end of line.
 */
_token
getToken(char *p)
{
	while(*p) {
		if (*p != ' ' && *p != '\t' && *p != '\n') {
			break;
		}
		p++;
	}
	char *q = p;
	while(*p) {
		if (*p == ' ' || *p == '\t' || *p == ';' || *p == '\n') {
			break;
		}
		p++;
	}
	*p++ = '\0';
	_token token;
	token.p = p;
	token.q = q;
	return token;
}

/**
 * Look up a keyword in the list of keywords. If found, call its 
 * processing function.
 */
char *
lookupKeyword(char *keyword, char *p)
{
	int numKeywords = sizeof(keywords) / sizeof(keywords[0]);
	int len = strlen(keyword);
	for(int i = 0; i < numKeywords; i++) {
		if (len == keywords[i].len && 
				(strncmp(keyword, keywords[i].keyword, len) == 0)) {
			printf("FOUND %s\n", keyword);
		}
		// keyword list is sorted by length, no need to keep looking if
		// remaining keywords are longer than the one we are looking for.
		if (len < keywords[i].len) {
			break;
		}
	}
	return p;
}
