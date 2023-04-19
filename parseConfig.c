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
int portOk(_server *);
void addPort(_server *);

typedef char * (*processKeyword)(char *);
typedef struct _keywords {
	char *keyword;
	int len;
	processKeyword func;
}_keywords;

// The following `f_` functions implement the config file keywords

// specify the pid file location
char *
f_pid(char *p) {
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

// specify option for network trace
char *
f_trace(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *trace = token.q;
	g.trace = (strcmp(trace, "on") == 0) ? 1 : 0;
	if (g.debug) {
		fprintf(stderr,"Show network trace %d\n", g.autoIndex);
	}
	return p;
}

// specify if directory listing should be allowed
char *
f_autoindex(char *p) {
	_token token = getToken(p);
	p = token.p;
	_server *server = g.servers;
	if (server == NULL) {
		fprintf(stderr, "'autoindex' directive outside a 'server' block, ignored\n");
	} else {
		char *autoIndex = token.q;
		server->autoIndex = (strcmp(autoIndex, "on") == 0) ? 1 : 0;
		if (g.debug) {
			fprintf(stderr,"Auto index %d\n", server->autoIndex);
		}
	}
	return p;
}

// specify if the `sendfile` system call should be used
char *
f_sendfile(char *p) {
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
f_user(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *user = token.q;
	g.user = (char *)malloc(strlen(user)+1);
	strcpy(g.user, user);
	if (g.debug) {
		fprintf(stderr,"Run as user name %s\n", g.user);
	}
	return p;
}

// process the server section
char *
f_server(char *p) {
	_server *server = (_server *)malloc(sizeof(_server));
	server->next = g.servers;
	server->port = g.defaultServer->port;	// default listen port
	server->tls = 0;
	g.servers = server;
	_token token = getSection(p);
	p = token.p;
	char *s = token.q;
	while (*s) {
		_token token = getToken(s);
		s = token.p;
		char *keyword = token.q;
		s = lookupKeyword(keyword, s);
	}
	return p;
}

// process the http section
char *
f_http(char *p) {
	_token token = getSection(p);
	p = token.p;
	char *s = token.q;
	while (*s) {
		_token token = getToken(s);
		s = token.p;
		char *keyword = token.q;
		s = lookupKeyword(keyword, s);
	}
	return p;
}

// document root
char *
f_root(char *p) {
	_token token = getToken(p);
	p = token.p;
	_server *server = g.servers;
	if (server == NULL) {
		fprintf(stderr, "'root' directive outside a 'server' block, ignored\n");
	} else {
		char *docRoot = token.q;
		server->docRoot = (char *)malloc(strlen(docRoot)+1);
		strcpy(server->docRoot, docRoot);
		if (g.debug) {
			fprintf(stderr,"Document root %s\n", server->docRoot);
		}
	}
	return p;
}

// server name
char *
f_server_name(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *serverName = token.q;
	_server *server = g.servers;
	if (server == NULL) {
		fprintf(stderr, "'server_name' directive outside a 'server' block, ignored\n");
	} else {
		server->serverName = (char *)malloc(strlen(serverName)+1);
		strcpy(server->serverName, serverName);
		if (g.debug) {
			fprintf(stderr,"server name: %s\n", server->serverName);
		}
	}
	return p;
}

// index file name
char *
f_indexFile(char *p) {
	if (g.indexFile) {
		free(g.indexFile);
		doDebug("Duplicate index file definition.");
	}
	_token token = getToken(p);
	p = token.p;
	char *indexFile = token.q;
	g.indexFile = (char *)malloc(strlen(indexFile)+1);
	strcpy(g.indexFile, indexFile);
	if (g.debug) {
		fprintf(stderr,"Index file name %s\n", g.indexFile);
	}
	return p;
}

// error log file path
char *
f_error_log(char *p) {
	if (g.errorLog) {
		free(g.errorLog);
		doDebug("Duplicate error log file definition.");
	}
	_token token = getToken(p);
	p = token.p;
	char *errorLog = token.q;
	g.errorLog = (char *)malloc(strlen(errorLog)+1);
	strcpy(g.errorLog, errorLog);
	if (g.debug) {
		fprintf(stderr,"Error log file path %s\n", g.errorLog);
	}
	return p;
}

// access log file path
char *
f_access_log(char *p) {
	if (g.accessLog) {
		free(g.accessLog);
		doDebug("Duplicate access log file definition.");
	}
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
		if (g.debug) {
			fprintf(stderr, "Access log, ignored \"%s\"\n", token.q);
		}
	}
	return p;
}

// ssl/tls certificate file
char *
f_ssl_certificate(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *q = token.q;
	if (*q == '"') {
		q++;
	}
	char *certFile = q;
	q = strchr(certFile, '"');
	if (q != NULL) {
		*q = '\0';
	}
	_server *server = g.servers;
	if (server == NULL) {
		fprintf(stderr, "'ssl_certificate' directive outside a 'server' block, ignored\n");
	} else {
		server->certFile = (char *)malloc(strlen(certFile)+1);
		strcpy(server->certFile, certFile);
		if (g.debug) {
			fprintf(stderr,"cert file: %s\n", server->certFile);
		}
	}
	return p;
	if (g.certFile) {
		free(g.certFile);
		doDebug("Duplicate cert file definition.");
	}
	return p;
}

// ssl/tls certificate key file
char *
f_ssl_certificate_key(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *q = token.q;
	if (*q == '"') {
		q++;
	}
	char *keyFile = q;
	q = strchr(keyFile, '"');
	if (q != NULL) {
		*q = '\0';
	}
	_server *server = g.servers;
	if (server == NULL) {
		fprintf(stderr, "'ssl_certificate_key' directive outside a 'server' block, ignored\n");
	} else {
		server->keyFile = (char *)malloc(strlen(keyFile)+1);
		strcpy(server->keyFile, keyFile);
		if (g.debug) {
			fprintf(stderr,"cert key file: %s\n", server->keyFile);
		}
	}
	return p;
}

// listen port
char *
f_listen(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *port = token.q;
	_server *server = g.servers;
	if (server == NULL) {
		fprintf(stderr, "'listen' directive outside a 'server' block, ignored\n");
		return p;
	}
	server->port = atoi(port);
	if (g.debug) {
		fprintf(stderr,"Listen on port: %d\n", server->port);
	}
	server->tls = 0;
	if (token.more) {
		token = getToken(p);
		p = token.p;
		if (strcmp(token.q, "ssl") == 0) {
			server->tls = 1;
		} else {
			doDebug("Unknown `listen` token ignored");
		}
	}
	return p;
}

// location directive
char *
f_location(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *tok = token.q;
	_server *server = g.servers;
	if (server == NULL) {
		fprintf(stderr, "'location' directive outside a 'server' block, ignored\n");
		return p;
	}
	_location *loc = (_location *)malloc(sizeof(_location));
	loc->type = TYPE_DOC_ROOT;	// default
	// A location starting with a slash implies a prefix operator.
	// Otherwise this token is an operator and a location token follows.
	if (*tok == '/') {
		loc->match = PREFIX_MATCH;
		loc->location = (char *)malloc(strlen(tok)+1);
		strcpy(loc->location, tok);
		if (g.debug) {
			fprintf(stderr,"Location prefix match: %s\n", loc->location);
		}
		loc->next = server->locations;
		server->locations = loc;
		loc->target = NULL;	// until the following token is parsed
	} else {
		if (!token.more) {
			doDebug("Location match missing the location, ignored");
			free(loc);
			return p;
		}
		if(*tok == '=') {
			loc->match = EXACT_MATCH;
		} else if (*tok == '~') {
			loc->match = REGEX_MATCH;
		} else {
			doDebug("Unrecognized location match, ignored");
			free(loc);
			return p;
		}
		token = getToken(p);
		loc->location = (char *)malloc(strlen(token.q)+1);
		strcpy(loc->location, token.q);
		p = token.p;
		loc->next = server->locations;
		server->locations = loc;
		loc->target = NULL;
	}
	// Location match tokens should be followed by a section
	// containing the location target.
	token = getSection(p);
	p = token.p;
	char *s = token.q;
	while (*s) {
		_token token = getToken(s);
		s = token.p;
		char *keyword = token.q;
		s = lookupKeyword(keyword, s);
	}
	return p;
}

// proxy_pass directive
char *
f_proxy_pass(char *p) {
	_token token = getToken(p);
	p = token.p;
	_server *server = g.servers;
	if (server == NULL) {
		fprintf(stderr, "'proxy_pass' directive outside a 'server' block, ignored\n");
		return p;
	}
	_location *loc = server->locations;
	if (loc == NULL) {
		fprintf(stderr, "'proxy_pass' directive outside a 'location' block, ignored\n");
		return p;
	}
	loc->type = TYPE_PROXY_PASS;
	loc->target = (char *)malloc(strlen(token.q)+1);
	strcpy(loc->target, token.q);
	if (g.debug) {
		fprintf(stderr,"Proxy pass: %s\n", loc->target);
	}
	return p;
}

// keepalive timeout
char *
f_keepalive_timeout(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *keepaliveTimeout = token.q;
	g.keepaliveTimeout = atoi(keepaliveTimeout);
	if (g.debug) {
		fprintf(stderr,"Keepalive timeout: %d\n", g.keepaliveTimeout);
	}
	return p;
}

// max worker processes
char *
f_workerProcesses(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *workerProcesses = token.q;
	g.workerProcesses = atoi(workerProcesses);
	if (g.debug) {
		fprintf(stderr,"Max worker processes %d\n", g.workerProcesses);
	}
	return p;
}

// max worker connections
char *
f_workerConnections(char *p) {
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
char *
f_events(char *p) {
	_token token = getSection(p);
	p = token.p;
	char *s = token.q;
	while (*s) {
		_token token = getToken(s);
		s = token.p;
		char *keyword = token.q;
		s = lookupKeyword(keyword, s);
	}
	return p;
}

_keywords keywords[] = {
	{"pid", 3, f_pid},
	{"user", 4, f_user},
	{"http", 4, f_http},
	{"root", 4, f_root},
	{"index", 5, f_indexFile},
	{"trace", 5, f_trace},
	{"events", 6, f_events},
	{"server", 6, f_server},
	{"listen", 6, f_listen},
	{"sendfile", 8, f_sendfile},
	{"location", 8, f_location},
	{"error_log", 9, f_error_log},
	{"autoindex", 9, f_autoindex},
	{"proxy_pass", 10, f_proxy_pass},
	//{"error_page", 10, 0},
	//{"log_format", 10, 0},
	{"access_log", 10, f_access_log},
	{"server_name", 11, f_server_name},
	//{"default_type", 12, 0},
	{"ssl_certificate", 15, f_ssl_certificate},
	{"worker_processes", 16, f_workerProcesses},
	{"keepalive_timeout", 17, f_keepalive_timeout},
	{"worker_connections", 18, f_workerConnections},
	{"ssl_certificate_key", 19, f_ssl_certificate_key}
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
	// parse the configuration
	while (*p) {
		_token token = getToken(p);
		p = token.p;
		char *keyword = token.q;
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
	// ignore leading whitespace
	while(*p) {
		if (*p != ' ' && *p != '\t' && *p != '\n') {
			break;
		}
		p++;
	}
	char *q = p;
	_token token;
	token.more = 0;
	token.p = p;
	token.q = q;
	if (*p == '\0') {
		return token;
	}
	while(*p) {
		if (*p == ' ' || *p == '\t' || *p == ';' || *p == '\n') {
			break;
		}
		p++;
	}
	if (*p != ';') {
		token.more = 1;
	}
	*p++ = '\0';
	token.p = p;
	return token;
}

/**
 * Get a section from the config file. A seection is a set of tokens 
 * delimited by { and }. It is possible to nest sections.
 */
_token
getSection(char *p)
{
	// ignore leading whitespace
	while(*p) {
		if (*p != ' ' && *p != '\t' && *p != '\n') {
			break;
		}
		p++;
	}
	if (*p != '{') {
		fprintf(stderr, "Invalid config file\n");
	}
	char *q = ++p;
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
	doDebug("Section not cloed with a `}`");
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

	for(_server *s = g.servers; s != NULL; s = s->next) {
		if (access(s->docRoot, R_OK) == -1) {
			perror("doc root not valid:");
			exit(1);
		}
	}
	if (access(g.defaultServer->docRoot, R_OK) == -1) {
		perror("default doc root not valid:");
		exit(1);
	}

	g.accessFd = open(g.accessLog, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
	if (g.accessFd == -1) {
		perror("access log not valid:");
		exit(1);
	}
	g.errorFd = open(g.errorLog, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
	if (g.errorFd == -1) {
		perror("error log not valid:");
		exit(1);
	}

	FILE *fp = fopen(g.pidFile, "r");
	if (fp == NULL) {
		perror("pid log not valid:");
		exit(1);
	} else {
		fclose(fp);
	}

	_server *server = g.servers;
	while(server != NULL) {
		if (!portOk(server)) {
			exit(1);
		}
		server = server->next;
	}
}

/**
 * return 1 = ok, 0 = not ok
 */
int
portOk(_server *server) {
	_ports *port = g.ports;
	while(port != NULL) {
		if (port->portNum == server->port) {
			if (port->useTLS != server->tls) {
				fprintf(stderr, "HTTP and HTTPS on the same port not supported, port %d\n", port->portNum);
			
				return 0;
			}
			return 1;
		}
		port = port->next;
	}
	// unique port/tls combination
	addPort(server);
	return 1;
}

void
addPort(_server *server) {
	_ports *port = (_ports *)malloc(sizeof(_ports));
	if (port == NULL) {
		perror("Out of memory");
		exit(1);
	}
	port->portNum = server->port;
	port->useTLS = server->tls;
	port->next = g.ports;
	g.ports = port;
	g.portCount++;
	return;
}
