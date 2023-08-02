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

static _logFile *accessLogs = NULL;
static _logFile *errorLogs = NULL;
static _index_file *indexFiles = NULL;
static _server_name *serverNames = NULL;
static _doc_root *docRoots = NULL;
static _server *servers = NULL;
static _port *ports = NULL;
static _location *locations = NULL;
static _try_targets tryTargets = NULL;
static char *certFile = NULL;
static char *keyFile = NULL;

void
defaultAccessLog() {
	_log_file *log = (_log_file *)malloc(sizeof(_log_file));
	log->path = "/var/log/ogws/access.log";
	log->next = NULL;
	accessLogs = log;
}

void
defaultErrorLog() {
	_log_file *log = (_log_file *)malloc(sizeof(_log_file));
	log->path = "/var/log/ogws/error.log";
	log->next = NULL;
	errorLogs = log;
}

void
defaultPort() {
	_port *p = (_port *)malloc(sizeof(_port));
	p->portNum = 8080;
	p->next = NULL;
	ports = p;
}

void
defaultServerName() {
	_server_name *sn = (_server_name *)malloc(sizeof(_server_name));
	sn->portNum = "*";
	sn->next = NULL;
	serverNames = sn;
}

void
defaultServer() {
	_server *server = (_server *)malloc(sizeof(_server));
	server->ports = NULL;
	server->tls = 0;
	server->certFile = NULL;
	server->keyFile = NULL;
	server->serverNames = NULL;
	server->locations = NULL;
	server->accessLog = NULL;
	server->errorLog = NULL;
	server->next = servers;
	servers = server;
	return;
}

void
defaultLocation() {
	_location *loc = (_location *)malloc(sizeof(_location));
	loc->type = TYPE_DOC_ROOT;
	loc->matchType = PREFIX_MATCH;
	loc->match = "/";
	loc->autoIndex = 0;
	loc->root = "/var/www/ogws";
	loc->try_target = NULL;
	loc->passTo = NULL;	
	loc->expires = 0;
	loc->next = locations;
	locations = loc;
	return;
}

void 
defaultIndexFile() {
	_index_file *index = (_index_file *)malloc(sizeof(_index_file));
	index->indexFile = "index.html";
	index->next = NULL;
	indexFiles = index;
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
f_root(char *path) {
	_doc_root *r = (_doc_root *)malloc(sizeof(_doc_root));
	r->path = path;
	r->next = docRoots;
	docRoots = r;
	if (g.debug) {
		fprintf(stderr,"Document root path %s\n", r->path);
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
int dupName(char *name) {
	_server_name *sn = pendingServer->serverNames;
	while(sn) {
		if ((strlen(sn->serverName) == strlen(name))
			&& (strcmp(sn->serverName, name) == 0)) {
			return 1;
		}
		sn = sn->next;
	}
	return 0;
}

void
f_server_name(char *serverName, int type) {
	if (!dupName(serverName)) {
		_server_name *sn = (_server_name *)malloc(sizeof(_server_name));
		sn->serverName = serverName;
		sn->type = type;
		sn->next = serverNames;
		serverNames = sn;
	}
	if (g.debug) {
		fprintf(stderr,"server name: %s\n", sn->serverName);
	}
	return;
}

// index file name
void
f_indexFile(char *indexFile) {
	_index_file *i = (_index_file *)malloc(sizeof(_index_file));
	i->next = indexFiles;
	indexFiles = i;
	i->indexFile = indexFile;
	if (g.debug) {
		fprintf(stderr, "Index File name: %s\n", i->indexFile);
	}
	return;
}

// error log file path
void
f_error_log(char *errorLog) {
	_log_file *log = (_log_file *)malloc(sizeof(_log_file));
	log->path = errorLog;
	log->fd = -1;
	log->next = errorLogs;
	errorLogs = log;
	if (g.debug) {
		fprintf(stderr,"Error log file path %s\n", log->errorLog);
	}
	return;
}

// access log file path
void
f_access_log(char *accessLog, int type) {
	_log_file *log = (_log_file *)malloc(sizeof(_log_file));
	log->path = accessLog;
	log->type = type;
	log->fd = -1;
	log->next = accessLogs;
	accessLogs = log;
	if (g.debug) {
		fprintf(stderr,"Access log file path %s\n", log->accessLog);
	}
	if (type) {
		fprintf(stderr, "Access log MAIN ignored\n");
	}
	return;
}

// ssl/tls certificate file
void
f_ssl_certificate(char *path) {
	if (certFile) {
		fprintf(stderr, "Duplicate cert file, ignored: %s\n", path);
		free(path);
		return;
	}
	certFile = path;
	if (g.debug) {
		fprintf(stderr,"cert file: %s\n", certFile);
	}
	return;
}

// ssl/tls certificate key file
void
f_ssl_certificate_key(char *path) {
	if (keyFile) {
		fprintf(stderr, "Duplicate key file, ignored: %s\n", path);
		free(path);
		return;
	}
	keyFile = path;
	if (g.debug) {
		fprintf(stderr,"key file: %s\n", keyFile);
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
int dupPort(int port) {
	_ports *p = pendingServer->ports;
	while(p) {
		if (p->port == port) {
			return 1;
		}
		p = p->next;
	}
	return 0;
}

void
f_listen(char *name, int port) {
	if (port > 0) {
		if (!dupPort(port)) {
			_ports *p = (_ports *)malloc(sizeof(_ports));
			p->port = port;
			p->next = ports;
			ports = p;
		}
	}
	if (name != NULL) {
		if (!dupName(name)) {
			_server_name *sn = (_server_name *)malloc(sizeof(_server_name));
			sn->serverName = name;
			sn->next = serverNames;
			serverNames = sn;
		}
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
	_location *loc = (_location *)malloc(sizeof(_location));
	loc->matchType = match;
	loc->type = type;
	loc->next = locations;
	locations = loc;
	return;
}

void
f_try_target(char *target) {
	locations->type = TYPE_TRY_FILES;
	_try_target *tt = malloc(sizeof(_try_target));
	_try_target *prev = pendingLocation->try_target;
	if (prev == NULL) {
		tryTargets->target = tt;
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
	doDebug("try_files directive complete");
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
	defaultAccessLog();
	defaultErrorLog();
	defaultPort();
	defaultServerName();
	defaultServer();
	defaultLocation();
	defaultIndexFile();

	yyin = fopen(g.configFile, "r");
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
