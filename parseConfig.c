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

typedef struct _token {
	char *p;
	char *q;
	int more;
}_token;

void removeComments(char *);
_token getToken(char *);
_token getSection(char *);
char * lookupKeyword(char *, char *);

typedef char * (*processKeyword)(char *);
typedef struct _keywords {
	char *keyword;
	int len;
	processKeyword func;
}_keywords;

// specify the pid file location
char *
pid(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *pidFile = token.q;
	g.pidFile = (char *)malloc(strlen(pidFile)+1);
	strcpy(g.pidFile, pidFile);
	if (g.debug) {
		fprintf(stderr,"PID file location:  %s\n", g.pidFile);
	}
	return p;
}

// specify is the `sendfile` system call should be used
char *
sendfile(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *sendfile = token.q;
	g.useSendfile = (strcmp(sendfile, "on") == 0) ? 1 : 0;
	if (g.debug) {
		fprintf(stderr,"Use sendfile %d\n", g.useSendfile);
	}
	return p;
}

// specify the user name to be used for the server process
char *
user(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *user = token.q;
	g.user = (char *)malloc(strlen(user));
	strcpy(g.user, user);
	if (g.debug) {
		fprintf(stderr,"Run as user name %s\n", g.user);
	}
	return p;
}

// process the server section
char *
_server(char *p) {
	_token token = getSection(p);
	p = token.p;
	char *s = token.q;
	fprintf(stderr,"server section\n");
	while (*s) {
		_token token = getToken(s);
		s = token.p;
		char *keyword = token.q;
		fprintf(stderr, "TOKEN %s\n", keyword);
		s = lookupKeyword(keyword, s);
	}
	return p;
}

// process the http section
char *
http(char *p) {
	_token token = getSection(p);
	p = token.p;
	char *s = token.q;
	fprintf(stderr,"http section\n");
	while (*s) {
		_token token = getToken(s);
		s = token.p;
		char *keyword = token.q;
		fprintf(stderr, "TOKEN %s\n", keyword);
		s = lookupKeyword(keyword, s);
	}
	return p;
}

// document root
char *
root(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *docRoot = token.q;
	g.docRoot = (char *)malloc(strlen(docRoot));
	strcpy(g.docRoot, docRoot);
	if (g.debug) {
		fprintf(stderr,"Document root %s\n", g.docRoot);
	}
	return p;
}

// server name
char *
server_name(char *p) {
	if (g.serverName) {
		free(g.serverName);
	}
	_token token = getToken(p);
	p = token.p;
	char *serverName = token.q;
	g.serverName = (char *)malloc(strlen(serverName));
	strcpy(g.serverName, serverName);
	if (g.debug) {
		fprintf(stderr,"Server Name: %s\n", g.serverName);
	}
	return p;
}

// index file name
char *
indexFile(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *indexFile = token.q;
	g.indexFile = (char *)malloc(strlen(indexFile));
	strcpy(g.indexFile, indexFile);
	if (g.debug) {
		fprintf(stderr,"Index file name %s\n", g.indexFile);
	}
	return p;
}

// error log file path
char *
error_log(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *errorLog = token.q;
	g.errorLog = (char *)malloc(strlen(errorLog));
	strcpy(g.errorLog, errorLog);
	if (g.debug) {
		fprintf(stderr,"Error log file path %s\n", g.errorLog);
	}
	return p;
}

// access log file path
char *
access_log(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *accessLog = token.q;
	g.accessLog = (char *)malloc(strlen(accessLog)+1);
	strcpy(g.accessLog, accessLog);
	if (g.debug) {
		fprintf(stderr,"Access log file path %s\n", g.accessLog);
	}
	while(token.more) {
		token = getToken(p);
		p = token.p;
		fprintf(stderr, "Access log, ignored \"%s\"\n", token.q);
	}
	return p;
}

// listen port
char *
_listen(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *port = token.q;
	g.port = atoi(port);
	if (g.debug) {
		fprintf(stderr,"Listen on port: %d\n", g.port);
	}
	return p;
}

// keepalive timeout
char *
keepalive_timeout(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *keepaliveTimeout = token.q;
	g.keepaliveTimeout = atoi(keepaliveTimeout);
	if (g.debug) {
		fprintf(stderr,"Keepalive timeout: %d\n", g.keepaliveTimeout);
	}
	return p;
}

// max worker connections
char *
workerConnections(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *workerConnections = token.q;
	g.workerConnections = atoi(workerConnections);
	if (g.debug) {
		fprintf(stderr,"Max worker connections %d\n", g.workerConnections);
	}
	return p;
}

// process the events section
char *events(char *p) {
	_token token = getSection(p);
	p = token.p;
	char *s = token.q;
	fprintf(stderr,"events section\n");
	while (*s) {
		_token token = getToken(s);
		s = token.p;
		char *keyword = token.q;
		fprintf(stderr, "TOKEN %s\n", keyword);
		s = lookupKeyword(keyword, s);
	}
	return p;
}

_keywords keywords[] = {
	{"pid", 3, pid},
	{"user", 4, user},
	{"http", 4, http},
	{"root", 4, root},
	{"index", 5, indexFile},
	{"events", 6, events},
	{"server", 6, _server},
	{"listen", 6, _listen},
	//{"location", 8, 1},
	{"sendfile", 8, sendfile},
	{"error_log", 9, error_log},
	//{"error_page", 10, 0},
	//{"log_format", 10, 0},
	{"access_log", 10, access_log},
	{"server_name", 11, server_name},
	//{"default_type", 12, 0},
	//{"ssl_certificate", 15, 0},
	{"keepalive_timeout", 17, keepalive_timeout},
	{"worker_connections", 18, workerConnections},
	//{"ssl_certificate_key", 19, 0}
};

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
		fprintf(stderr, "TOKEN %s\n", keyword);
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
	_token token;
	token.more = 1; //default: more than one parameter
	while(*p) {
		if (*p == ' ' || *p == '\t' || *p == ';' || *p == '\n') {
			break;
		}
		p++;
	}
	if (*p == ';') {
		token.more = 0;
	}
	*p++ = '\0';
	token.p = p;
	token.q = q;
	return token;
}

/**
 * Get a section from the config file. A seection is a set of tokens 
 * delimited by { and }. It is possible to nest sections.
 */
_token
getSection(char *p)
{
	while(*p) {
		if (*p != ' ' && *p != '\t' && *p != '\n') {
			break;
		}
		p++;
	}
	char *q = p;
	if (*p != '{') {
		fprintf(stderr, "Invalid config file\n");
	}
	int nest = 1;
	while(*p) {
		if (*p == '}') {
			nest--;
			if (nest == 0) {
				*p++ = '\0';
				_token token;
				token.p = p;
				token.q = q;
				return token;
			}
		}
		if (*p == '{') {
			nest++;
		}
		p++;
	}
	_token token;
	token.p = p;
	token.q = q;
	token.more = 0;
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
			p = (*keywords[i].func)(p);
		}
		// keyword list is sorted by length, no need to keep looking if
		// remaining keywords are longer than the one we are looking for.
		if (len < keywords[i].len) {
			break;
		}
	}
	return p;
}

/**
 * Check sanity of the configuration
 */
void
checkConfig()
{
	if (access(g.configPath, R_OK) == -1) {
		perror("config path not valid:");
		exit(1);
	}

	if (access(g.docRoot, R_OK) == -1) {
		perror("doc root not valid:");
		exit(1);
	}

	FILE* fp = fopen(g.accessLog, "w+");
	if (fp == NULL) {
		perror("access log not valid:");
		exit(1);
	} else {
		fclose(fp);
	}

	fp = fopen(g.errorLog, "w+");
	if (fp == NULL) {
		perror("error log not valid:");
		exit(1);
	} else {
		fclose(fp);
	}

	fp = fopen(g.pidFile, "w+");
	if (fp == NULL) {
		perror("pid log not valid:");
		exit(1);
	} else {
		fclose(fp);
	}
}
