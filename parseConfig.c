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
#include <netdb.h>
#include <sys/types.h>
#include "server.h"
#include "global.h"

int portOk(_server *);
void addPort(_server *);
int locationSet(_server *);

// The following `f_` functions implement the config file keywords
// The functions are called from the lex/yacc generated code as the 
// config file is parsed.

// specify the pid file location
void
f_pid(char *pidFile) {
	g.pidFile = pidFile;
	if (g.debug) {
		fprintf(stderr,"PID file location:  %s\n", g.pidFile);
	}
}

// specify option for network trace
void
f_trace(int trace) {
	g.trace = trace;
	if (g.debug) {
		if (trace) {
			fprintf(stderr,"Show network trace ON\n", );
		} else {
			fprintf(stderr,"Show network trace OFF\n", );
		}
	}
}

// specify if directory listing should be allowed
void
f_autoindex(int autoIndex) {
	g.autoIndex = autoIndex;
	if (g.debug) {
		if (autoIndex) {
			fprintf(stderr,"Directory index ON\n", );
		} else {
			fprintf(stderr,"Directory index OFF\n", );
		}
	}
}

// specify if the `sendfile` system call should be used
void
f_sendfile(int sendFile) {
	g.sendFile = sendFile;
	if (g.debug) {
		if (sendFile) {
			fprintf(stderr,"Use sendfile system call ON\n", );
		} else {
			fprintf(stderr,"Use sendfile system call OFF\n", );
		}
	}
}

// specify if the `tcp_nopush` option
void
f_tcpnopush(int tcpNoPush) {
	g.tcpNoPush = tcpNoPush;
	if (g.debug) {
		if (tcpNoPush) {
			fprintf(stderr,"Use `tcp_nopush` option ON\n", );
		} else {
			fprintf(stderr,"Use `tcp_nopush` option OFF\n", );
		}
	}
}

// specify the user name to be used for the server process
void
f_user(char *user, char *group) {
	if (g.user) {
		free(g.user);
	}
	g.user = user;
	if (g.debug) {
		fprintf(stderr,"Run as user name %s\n", g.user);
	}
	if (group) {
		if (g.group) {
			free(g.group);
		}
		g.group = group;
		if (g.debug) {
			fprintf(stderr,"Run as group name %s\n", g.group);
		}
	}
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
		loc->passTo = NULL; 
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

// try_files directive
//
// note: only one variation is currently supported:
// try_files $uri $uri/ <target>
//
char *
f_try_files(char *p) {
	_token token = getToken(p);
	p = token.p;
	char *tok = token.q;
	_server *server = g.servers;
	if (server == NULL) {
		fprintf(stderr, "'try_files' directive outside a 'server' block, ignored\n");
		return p;
	}
	_location *loc = server->locations;
	if (loc == NULL) {
		fprintf(stderr, "'try_files' directive outside a 'location' block, ignored\n");
		return p;
	}
	loc->type = TYPE_TRY_FILES;
	loc->match = PREFIX_MATCH;
	if (strcmp(tok, "$uri") != 0) {
		fprintf(stderr, "'try_files' wrong format 1, ignored\n");
		return p;
	}
	if (!token.more) {
		fprintf(stderr, "'try_files' wrong format 2, ignored\n");
		return p;
	}
	token = getToken(p);
	p = token.p;
	tok = token.q;
	if (strcmp(tok, "$uri/") != 0) {
		fprintf(stderr, "'try_files' wrong format 3, ignored\n");
		return p;
	}
	if (!token.more) {
		fprintf(stderr, "'try_files' wrong format 4, ignored\n");
		return p;
	}
	token = getToken(p);
	p = token.p;
	tok = token.q;
	loc->target = (char *)malloc(strlen(tok)+1);
	strcpy(loc->target, tok);
	if (g.debug) {
		fprintf(stderr,"Try_files: %s\n", loc->target);
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
	char *hn = strstr(token.q, "://");
	if (hn) {
		hn += 3;
	}
	char *pn = strchr(hn, ':');
	unsigned short port;
	if (pn) {
		*pn++ = '\0';
		port = (unsigned short)atoi(pn);
	}else {
		port = 80;
	}
	loc->target = (char *)malloc(strlen(hn)+1);
	strcpy(loc->target, hn);
	if (g.debug) {
		fprintf(stderr,"Proxy pass: %s\n", loc->target);
	}
	struct sockaddr_in *passTo = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	memset(passTo, 0, sizeof(struct sockaddr_in));
	loc->passTo = passTo;
	struct hostent *hostName = gethostbyname(hn);
	if (hostName == (struct hostent *)0) {
		doDebug("gethostbyname failed");
		exit(1);
	}
	passTo->sin_family = AF_INET;
	passTo->sin_port = htons(port);
	passTo->sin_addr.s_addr = *((unsigned long*)hostName->h_addr);
	return p;
}

// keepalive timeout
void
f_keepalive_timeout(int timeout) {
	g.keepaliveTimeout = timeout;
	if (g.debug) {
		fprintf(stderr,"Keepalive timeout: %d\n", g.keepaliveTimeout);
	}
}

// max worker processes
void
f_workerProcesses(int workerProcesses) {
	g.workerProcesses = workerProcesses;
	if (g.debug) {
		fprintf(stderr,"Max worker processes %d\n", g.workerProcesses);
	}
}

// max worker connections
void
f_workerConnections(nt workerConnections) {
	g.workerConnections = workerConnections;
	if (g.debug) {
		fprintf(stderr,"Max worker connections %d\n", g.workerConnections);
	}
}

// interface to generated code from yacc/lex
extern FILE *yyin;
int yyparse (void);

void
parseConfig() {
	char fn[256];
	char *fileName = (char *)&fn;
	strcpy(fileName, g.configPath);
	strcat(fileName, "/ogws.conf");
	yyin = fopen(fileName, "r");
	if (yyin == NULL) {
		int e = errno;
		char buffer[BUFF_SIZE];
		snprintf(buffer, BUFF_SIZE, "%s: file open failed: %s\n", fileName, strerror(e));
		doDebug(buffer);
		exit(1);
	}
	yyparse();
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
			doDebug("no port set for a server.");
			exit(1);
		}
		if (!locationSet(server)) {
			doDebug("server does not have a location.");
			exit(1);
		}
		server = server->next;
	}
}

/**
 * return 1 = ok, 0 = not ok
 */
int
portOk(_server *server)
{
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
addPort(_server *server)
{
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

int
locationSet(_server *server)
{
	int ok = 1;		// TODO: implement better checking logic
	while(server) {
		int needsDefaultRoot = 1;
		_location *loc = server->locations;
		if (loc) {
			if (loc->type == TYPE_DOC_ROOT &&
					loc->match == PREFIX_MATCH &&
					strcmp(loc->location, "/") == 0) {
				needsDefaultRoot = 0;
				break;
			}
			loc = loc->next;
		}
		if (needsDefaultRoot) {
			// add a default location
			_location *loc = (_location *)malloc(sizeof(_location));
			loc->type = TYPE_DOC_ROOT;	// default
			loc->match = PREFIX_MATCH;
			char tok[] = "/";
			loc->location = (char *)malloc(strlen(tok)+1);
			strcpy(loc->location, tok);
			loc->passTo = NULL; 
			loc->type = TYPE_DOC_ROOT;
			if (server->docRoot == NULL) {
				loc->target = g.defaultServer->docRoot;
			} else {
				loc->target = server->docRoot;
			}
			// add the default prefix match as the last in the chain
			_location *prev = NULL;
			_location *l = server->locations;
			while(l) {
				prev = l;
				l = l->next;
			}
			if (prev) {
				prev->next = loc;
			} else {
				server->locations = loc;
			}
			loc->next = NULL;
		}
		server = server->next;
	}
	return ok;
}
