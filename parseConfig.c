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

static char *pendingProtocol = NULL;
static _server *pendingServer = NULL;
static _location *pendingLocation = NULL;

void
newPendingServer() {
	_server *ps = (_server *)malloc(sizeof(_server));
	_ports *p = (_ports *)malloc(sizeof(_ports));
	p->port = 8080;
	p->next = NULL;
	ps->ports = p;
	ps->tls = 0;
	ps->certFile = NULL;
	ps->keyFile = NULL;
	ps->serverNames = NULL;
	ps->locations = NULL;
	ps->accessLog = NULL;
	ps->errorLog = NULL;
	ps->next = pendingServer;
	pendingServer = ps;
	return;
}

void
newPendingLocation() {
	_location *pl = (_location *)malloc(sizeof(_location));
	pl->type = TYPE_DOC_ROOT;
	pl->match = NULL;
	pl->autoIndex = 0;
	pl->root = NULL;
	pl->try_target = NULL;
	pl->passTo = NULL;		// for proxy_pass locations
	pl->expires = 0;
	pl->next = pendingLocation;
	pendingLocation = pl;
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
			fprintf(stderr,"Show network trace ON\n");
		} else {
			fprintf(stderr,"Show network trace OFF\n");
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
			fprintf(stderr,"Use sendfile system call ON\n");
		} else {
			fprintf(stderr,"Use sendfile system call OFF\n");
		}
	}
}

// specify if the `tcp_nopush` option
void
f_tcpnopush(int tcpNoPush) {
	g.tcpNoPush = tcpNoPush;
	if (g.debug) {
		if (tcpNoPush) {
			fprintf(stderr,"Use `tcp_nopush` option ON\n");
		} else {
			fprintf(stderr,"Use `tcp_nopush` option OFF\n");
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
		exit(1);
	}
	if (pendingServer->locations != NULL) {
		fprintf(stderr, "Location block outside server block, bad config\n");
		exit(1);
	}
	// fill in default server configuration, which is used
	// if there are no server blocks or the URL does not match
	// the criteria in any server block.
	newPendingLocation();

	char docRoot[] = "/var/www/ogws/html";
	pendingLocation->root = (char *)malloc(strlen(docRoot)+1);
	strcpy(pendingLocation->root, docRoot);
	pendingServer->locations = pendingLocation;

	_server_name *sn = (_server_name *)malloc(sizeof(_server_name));
	sn->next = NULL;
	sn->serverName = "_";
	sn->type = SERVER_NAME_EXACT;
	pendingServer->serverNames = sn;

	_index_file *i = (_index_file *)malloc(sizeof(_index_file));
	i->next = NULL;
	i->indexFile = "index.html";
	pendingServer->indexFiles = i;

	char accessLog[] = "/var/log/ogws/access.log";
	pendingServer->accessLog = (char *)malloc(strlen(accessLog)+1);
	strcpy(pendingServer->accessLog, accessLog);

	char errorLog[] = "/var/log/ogws/error.log";
	pendingServer->errorLog = (char *)malloc(strlen(errorLog)+1);
	strcpy(pendingServer->errorLog, errorLog);

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
		int c = expires[len-1];
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
		fprintf(stderr,"Expires directive %d\n", pendingLocation->expires);
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
		fprintf(stderr, "Access log MAIN ignored\n");
	}
	return;
}

// ssl/tls certificate file
void
f_ssl_certificate(char *certFile) {
	if (pendingServer == NULL) {
		newPendingServer();
	}
	if (pendingServer->certFile) {
		fprintf(stderr, "Duplicate cert file, ignored: %s\n", certFile);
		free(certFile);
		return;
	}
	pendingServer->certFile = certFile;
	if (g.debug) {
		fprintf(stderr,"cert file: %s\n", pendingServer->certFile);
	}
	return;
}

// ssl/tls certificate key file
void
f_ssl_certificate_key(char *keyFile) {
	if (pendingServer == NULL) {
		newPendingServer();
	}
	if (pendingServer->keyFile) {
		fprintf(stderr, "Duplicate key file, ignored: %s\n", keyFile);
		free(keyFile);
		return;
	}
	pendingServer->keyFile = keyFile;
	if (g.debug) {
		fprintf(stderr,"key file: %s\n", pendingServer->keyFile);
	}
	return;
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
		_ports *p = (_ports *)malloc(sizeof(_ports));
		p->port = port;
		p->next = pendingServer->ports;
		pendingServer->ports = p;
	}
	if (name != NULL) {
		_server_name *sn = (_server_name *)malloc(sizeof(_server_name));
		sn->serverName = name;
		sn->next = pendingServer->serverNames;
		pendingServer->serverNames = sn;
	}
	return;
}

void
f_tls() {
	if (pendingServer == NULL) {
		fprintf(stderr, "SSL directive out of context, ignored\n");
		return;
	}
	pendingServer->tls = 1;
}

// location directive
void
f_location(int type, char *match) {
	if (pendingServer == NULL) {
		fprintf(stderr, "Location directive with no server, ignored\n");
		return;
	}
	_location *loc = pendingLocation;
	loc->match = match;
	loc->type = type;
	// pop the pending location stack and chain the location to the server
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
	tt->target = target;
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
	return;
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
f_workerConnections(int workerConnections) {
	g.workerConnections = workerConnections;
	if (g.debug) {
		fprintf(stderr,"Max worker connections %d\n", g.workerConnections);
	}
}

// config file parse successfully
void
f_config_complete() {
	checkConfig();
	if (g.debug) {
		fprintf(stderr, "Config file parsing complete!\n");
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
checkDocRoots(_server *s) {
	for(_location *loc = s->locations; loc != NULL; loc = loc->next) {
		if (access(loc->root, R_OK) == -1) {
			fprintf(stderr, "%s: ", loc->root);
			perror("doc root not valid:");
			exit(1);
		}
	}
}

void
checkServerNames(_server *s) {
	if (s->serverNames == NULL) {
		perror("missing server name");
		exit(1);
	}
}

void
checkIndexFiles(_server *s) {
	if (s->indexFiles == NULL) {
		perror("missing default index file");
		exit(1);
	}
}

void
checkAccessLogs(_server *s) {
	if (s->accessLog == NULL) {
		// use default
		if (g.servers->accessLog == NULL) {
			perror("missing access log");
			exit(1);
		}
		s->accessFd = g.servers->accessFd;
	} else {
		s->accessFd = open(s->accessLog, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
		if (s->accessFd == -1) {
			fprintf(stderr, "%s: ", s->accessLog);
			perror("access log not valid:");
			exit(1);
		}
	}
}

void
checkErrorLogs(_server *s) {
	if (s->errorLog == NULL) {
		// use default
		if (g.servers->errorLog == NULL) {
			perror("missing error log");
			exit(1);
		}
		s->errorFd = g.servers->errorFd;
	} else {
		s->errorFd = open(s->errorLog, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
		if (s->errorFd == -1) {
			fprintf(stderr, "%s: ", s->errorLog);
			perror("error log not valid:");
			exit(1);
		}
	}
}

void
checkCertFile(_server *s) {
	if (access(s->certFile, R_OK) == -1) {
		fprintf(stderr, "%s: ", s->certFile);
		perror("certificate file not valid:");
		exit(1);
	}
}

void
checkKeyFile(_server *s) {
	if (access(s->keyFile, R_OK) == -1) {
		fprintf(stderr, "%s: ", s->keyFile);
		perror("key file not valid:");
		exit(1);
	}
}

void
checkServers() {
	for(_server *s = g.servers; s != NULL; s = s->next) {
		if (!portOk(s)) {
			exit(1);
		}
		checkDocRoots(s);
		checkServerNames(s);
		checkIndexFiles(s);
		checkAccessLogs(s);
		checkErrorLogs(s);
		if (s->tls) {
			checkCertFile(s);
			checkKeyFile(s);
		}
	}
}

void
checkConfig()
{
	FILE *fp = fopen(g.pidFile, "r");
	if (fp == NULL) {
		perror("pid log not valid:");
		exit(1);
	} else {
		fclose(fp);
	}

	checkServers();
}

/**
 * Check for a port defined as TLS for one server and non-TLS for another.
 * Return 1 = ok, 0 = not ok
 */
int
checkPorts(int port, int tls) {
	_server *s= g.servers;
	while(s) {
		_ports *p = s->ports;
		while(p) {
			if ((p->port == port)
				&& (s->tls != tls)) {
				return 0;
			}
			p = p->next;
		}
		s = s->next;
	}
	return 1;
}

int
portOk(_server *server)
{
	_server *s= g.servers;
	while(s) {
		_ports *p = server->ports;
		while(p) {
			if (checkPorts(p->port, s->tls) == 0) {
				fprintf(stderr, "HTTP and HTTPS on the same port not supported, port %d\n", p->port);
				return 0;
			}
		}
		s = s->next;
	}
	// unique port/tls combination
	return 1;
}
