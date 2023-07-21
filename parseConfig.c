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

static char *pendingProtocol = NULL;
static _server *pendingServer = NULL;
static _location *pendingLocation = NULL;

void
newPendingServer() {
	pendingServer = (_server *)malloc(sizeof(_server));
	pendingServer->next = NULL;
	pendingServer->port = g.defaultServer->port;
	pendingServer->tls = 0;
	pendingServer->autoIndex = 0;
	pendingServer->certFile = NULL;
	pendingServer->keyFile = NULL;
	pendingServer->serverNames = NULL;
	pendingServer->locations = NULL;
	pendingServer->accessLog = NULL;
	pendingServer->errorLog = NULL;
	return;
}

void
newPendingLocation() {
	pendingLocation = (_location *)malloc(sizeof(_location));
	pendingLocation->next = pendingLocation;
	pendingLocation->type = TYPE_DOC_ROOT;
	pendingLocation->match = EXACT_MATCH;
	pendingLocation->autoIndex = 0;
	pendingLocation->location = NULL;
	pendingLocation->root = NULL;
	pendingLocation->try_target = NULL;
	pendingLocation->passTo = NULL;		// for proxy_pass locations
	return;
}

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
	if (pendingLocation == NULL) {
		newPendingLocation();
	}
	pendingLocation->autoIndex = autoIndex;
	if (g.debug) {
		fprintf(stderr,"Auto index %s\n", pendingLocation->autoIndex ? "On" : "Off");
	}
	return;
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
void
f_server() {
	if (pendingServer == NULL) {
		fprintf(stderr, "Empty server block, ignored\n");
	}
	// if there are pending location directives, apply them
	// at the server block level
	if (pendingLocation) {
		_location *loc = pendingLocation;
		pendingLocation = loc->next;
		// put the server default location behind the specific locations, if any
		if (pendingServer->locations == NULL) {
			pendingServer->locations = loc;
		} else {
			_location *prev = pendingServer->locations;
			while(prev->next != NULL) {
				prev = prev->next;
			}
			prev->next = loc;
		}
		loc->next = NULL;
	}
	// pop the pending server stack and chain the server block
	_server *server = pendingServer;
	pendingServer = server->next;
	server->next = g.servers;
	g.servers = server;
	return;
}

// process the http section
void
f_http() {
	if (pendingServer == NULL) {
		fprintf(stderr, "Missing server block, bad config\n");
	}
	// pop the pending server stack and chain the server block
	_server *server = pendingServer;
	pendingServer = server->next;
	server->next = g.servers;
	g.servers = server;
	return;
}

// document root
void
f_root(char *root) {
	if (pendingServer == NULL) {
		newPendingServer();
	}
	if (pendingLocation == NULL) {
		newPendingLocation();
	} else if(pendingLocation->root != NULL) {
		newPendingLocation();	// nested roots
	}
	pendingLocation->root = root;
	if (g.debug) {
		fprintf(stderr,"Document root path %s\n", pendingLocation->root);
	}
	return;
}

// expires directive
void
f_expires(char *expires) {
	if (pendingServer == NULL) {
		newPendingServer();
	}
	if (pendingLocation == NULL) {
		newPendingLocation();
	} else if(pendingLocation->root != NULL) {
		newPendingLocation();	// nested roots
	}
	if (strncmp(expires, "off", 3) == 0) {
		pendingLocation->expires = 0;
	} else {
		// format [0-9]+ or [0-9]+[hms]
		int len = strlen(expires);
		int c = expires[len-1]);
		if (isdigit(c)) {
			pendingLocation->expires = atoi(expires);
		} else {
			int mult = 1;
			if (c == 'm') {
				mult *= 60;
			} else if (c == 'h') {
				mult *= 60*60;
			} else {
				printf("Unknown unit, ignored\n");
			}
			expires[len-1] = '\0';
			pendingLocation->expires = atoi(expires) * mult;
		}
	}
	if (g.debug) {
		fprintf(stderr,"Expires directive %n\n", pendingLocation->expires);
	}
	return;
}

// server name
void
f_server_name(char *serverName, int type) {
	if (pendingServer == NULL) {
		newPendingServer();
	}
	_server_name *sn = (_server_name *)malloc(sizeof(_server_name));
	sn->serverName = serverName;
	sn->type = type;
		sn->next = pendingServer->serverNames;
		pendingServer->serverNames = sn;
		if (g.debug) {
			fprintf(stderr,"server name: %s\n", sn->serverName);
		}
	}
	return;
}

// index file name
void
f_indexFile(char *indexFile) {
	if (pendingServer == NULL) {
		newPendingServer();
	}
	_index_file *i = (_index_file *)malloc(sizeof(_index_file));
	i->next = pendingServer->indexFiles;
	pendingServer->indexFiles = i;
	i->indexFile = indexFile;
	if (g.debug) {
		fprintf(stderr, "Index File name: %s\n", indexFile);
	}
	return;
}

// error log file path
void
f_error_log(char *errorLog) {
	if (pendingServer == NULL) {
		newPendingServer();
	}
	pendingServer->errorLog = errorLog;
	if (g.debug) {
		fprintf(stderr,"Error log file path %s\n", pendingServer->errorLog);
	}
	return;
}

// access log file path
void
f_access_log(char *accessLog, int type) {
	if (pendingServer == NULL) {
		newPendingServer();
	}
	pendingServer->accessLog = accessLog;
	if (g.debug) {
		fprintf(stderr,"Access log file path %s\n", pendingServer->accessLog);
	}
	if (type) {
		fprintf("Access log MAIN ignored\n");
	}
	return;
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

// listen directive
// 	listen ip_address
// 	listen ip_address:port
// 	listen port
// 	listen localhost
// 	listen localhost:port
// 	listen *
// 	listen *:port
//
// 	additional options - defaultserver ssl http2
void
f_listen(char *name, int port) {
	if (pendingServer == NULL) {
		newPendingServer();
	}
	if (port > 0) {
		pendingServer->port;
	}
	if (name != NULL) {
		pendingServer->listen = name;
	}
	return;
}

void
f_tls() {
	if (pendingServer == NULL) {
		fprintf("SSL directive out of context, ignored\n");
		return;
	}
	pendingServer->tls = 1;
}

// location directive
void
f_location(int type, char *match, char *path) {
	if (pendingServer == NULL) {
		fprintf(stderr, "Location directive with no server, ignored\n");
		return;
	}
	// pop the pending location stack and chain the location to the server
	_locations *loc = pendingLocation;
	pendingLocation = pendingLocation->next;
	loc->next = pendingServer->locations;
	pendingServer->locations = loc;
	return;
}

void
f_try_target(char *target) {
	_server *server = g.servers;
	if (server == NULL) {
		fprintf(stderr, "'try_files' directive outside a 'server' block, ignored\n");
		return;
	}
	_location *loc = server->locations;
	if (loc == NULL) {
		fprintf(stderr, "'try_files' directive outside a 'location' block, ignored\n");
		return;
	}
	loc->type = TYPE_TRY_FILES;
	_try_target *tt = malloc(sizeof(_try_target));
	_try_target *prev = loc->try_target;
	if (prev == NULL) {
		loc->try_target = tt;
		tt->next = NULL;
	} else {
		while(prev->next) {
			prev = prev->next;
		}
		prev->next = tt;
		tt->next = NULL;
	}
}

// try_files directive
void
f_try_files() {
	_server *server = g.servers;
	if (server == NULL) {
		fprintf(stderr, "'try_files' directive outside a 'server' block, ignored\n");
		return;
	}
	_location *loc = server->locations;
	if (loc == NULL) {
		fprintf(stderr, "'try_files' directive outside a 'location' block, ignored\n");
		return;
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

// proxy_pass protocol
void
f_protocol(char *protocol) {
	if (pendingProtocol != NULL) {
		fprintf(stderr,"Duplicate protocol, ignored\n");
		return;
	}
	pendingProtocol = protocol;
	if (g.debug) {
		fprintf(stderr, "Proxy pass to %s protocol\n", pendingProtocol);
	}
	return;
}

// proxy_pass directive
void
f_proxy_pass(char *host, int port) {
	if (pendingLocation == NULL) {
		newPendingLocation();
	}
	struct sockaddr_in *passTo = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	memset(passTo, 0, sizeof(struct sockaddr_in));
	struct hostent *hostName = gethostbyname(host);
	if (hostName == (struct hostent *)0) {
		doDebug("gethostbyname failed");
		exit(1);
	}
	passTo->sin_family = AF_INET;
	passTo->sin_port = htons(port);
	passTo->sin_addr.s_addr = *((unsigned long*)hostName->h_addr);
	pendingLocation->passTo = passTo;
	return;
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
