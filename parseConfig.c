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
#include "serverlist.h"
#include "server.h"
#include "global.h"

int portOk(_server *);

static _index_file *indexFiles = NULL;
static _server_name *serverNames = NULL;
static _port *ports = NULL;
static _location *locations = NULL;
static _try_target *tryTargets = NULL;
static char *certFile = NULL;
static char *keyFile = NULL;
static int tls = 0;
static int autoIndex = 0;
static int protocol = 0;

void
defaultAccessLog() {
	_log_file *log = (_log_file *)malloc(sizeof(_log_file));
	log->path = "/var/log/ogws/access.log";
	log->next = NULL;
	g.accessLogs = log;
}

void
defaultErrorLog() {
	_log_file *log = (_log_file *)malloc(sizeof(_log_file));
	log->path = "/var/log/ogws/error.log";
	log->next = NULL;
	g.errorLogs = log;
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
	sn->serverName = "*";
	sn->next = NULL;
	serverNames = sn;
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

void
openLogFiles() {
	_log_file *log = g.accessLogs;
	while(log) {
		log->fd = open(log->path, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
		if (log->fd == -1) {
			fprintf(stderr, "%s: ", log->path);
			perror("access log not valid:");
			exit(1);
		}
	}
	log = g.errorLogs;
	while(log) {
		log->fd = open(log->path, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
		if (log->fd == -1) {
			fprintf(stderr, "%s: ", log->path);
			perror("error log not valid:");
			exit(1);
		}
	}
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
f_autoindex(int flag) {
	autoIndex = flag;
	if (g.debug) {
		fprintf(stderr,"Auto index %s\n", autoIndex ? "On" : "Off");
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
	_server *server = (_server *)malloc(sizeof(_server));
	server->next = g.servers;
	g.servers = server;
	_server_name *sn = serverNames;
	while(sn) {
		// pop the server name unless it is the default
		if (sn->next) {
			serverNames = sn->next;
		}
		sn->next = server->serverNames;
		server->serverNames = sn;
		sn = sn->next;
	}
	_index_file *i = indexFiles;
	while(i) {
		// pop the index file name unless it is the default
		if (i->next) {
			indexFiles = i->next;
		}
		i->next = server->indexFiles;
		server->indexFiles = i;
		i = i->next;
	}
	_port *port = ports;
	while(port) {
		// pop the port unless it is the default
		if (port->next) {
			ports = port->next;
		}
		port->next = server->ports;
		server->ports = port;
		port = port->next;
	}
	_location *loc = locations;
	while(loc) {
		// pop the location unless it is the default
		if (loc->next) {
			locations = loc->next;
		}
		loc->next = server->locations;
		server->locations = loc;
		loc = loc->next;
	}
	server->tls = tls;
	server->autoIndex = autoIndex;
	server->certFile = certFile;
	server->keyFile = keyFile;
	// reset defaults
	tls = 0;
	autoIndex = 0;
	certFile = NULL;
	keyFile = NULL;
	// pick the top of the stack but don't pop
	server->accessLog = g.accessLogs;
	server->errorLog = g.errorLogs;
	return;
}

// process the http section
void
f_http() {
	if (!g.servers) {
		if (g.debug) {
			doDebug("Missing server block, using default\n");
		}
	}
	// create a default server, even if there are servers, in case one of
	// them didn't define all the defaults.
	_server *server = (_server *)malloc(sizeof(_server));
	server->next = g.servers;
	g.servers = server;
	_server_name *sn = serverNames;
	server->serverNames = sn;
	_index_file *i = indexFiles;
	server->indexFiles = i;
	_port *port = ports;
	server->ports = port;
	_location *loc = locations;
	server->locations = loc;
	server->tls = 0;
	server->autoIndex = 0;
	server->certFile = certFile;
	server->keyFile = keyFile;
	server->accessLog = g.accessLogs;
	server->errorLog = g.errorLogs;
	return;
}

// document root
void
f_root(char *root) {
	_location *loc = (_location *)malloc(sizeof(_location));
	loc->next = locations;
	locations = loc;
	loc->root = root;
	if (g.debug) {
		fprintf(stderr,"Document root path %s\n", loc->root);
	}
	return;
}

// expires directive
void
f_expires(char *expires) {
	if (!locations) {
		fprintf(stderr, "Expires directive with no Location, ignored\n");
		return;
	}
	if (strncmp(expires, "off", 3) == 0) {
		locations->expires = 0;
	} else {
		// format [0-9]+ or [0-9]+[hms]
		int len = strlen(expires);
		int c = expires[len-1];
		if (isdigit(c)) {
			locations->expires = atoi(expires);
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
			locations->expires = atoi(expires) * mult;
		}
	}
	if (g.debug) {
		fprintf(stderr,"Expires directive %d\n", locations->expires);
	}
	return;
}

// server name
int dupName(char *name) {
	_server_name *sn = serverNames;
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
		fprintf(stderr,"server name: %s\n", serverName);
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
f_error_log(char *path) {
	_log_file *log = (_log_file *)malloc(sizeof(_log_file));
	log->path = path;
	log->fd = -1;
	log->next = g.errorLogs;
	g.errorLogs = log;
	if (g.debug) {
		fprintf(stderr,"Error log file path %s\n", log->path);
	}
	return;
}

// access log file path
void
f_access_log(char *path, int type) {
	_log_file *log = (_log_file *)malloc(sizeof(_log_file));
	log->path = path;
	log->type = type;
	log->fd = -1;
	log->next = g.accessLogs;
	g.accessLogs = log;
	if (g.debug) {
		fprintf(stderr,"Access log file path %s\n", log->path);
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
int dupPort(int portNum) {
	_port *p = ports;
	while(p) {
		if (p->portNum == portNum) {
			return 1;
		}
		p = p->next;
	}
	return 0;
}

void
f_listen(char *name, int portNum) {
	if (portNum > 0) {
		if (!dupPort(portNum)) {
			_port *p = (_port *)malloc(sizeof(_port));
			p->portNum = portNum;
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
	tls = 1;
}

// location directive
void
f_location(int type, char *match) {
	_location *loc = locations;
	loc->matchType = type;
	loc->match = match;
	locations = loc->next;
	_server *server = g.servers;
	if (server) {
		loc->next = server->locations;
		server->locations = loc;
	} else {
		fprintf(stderr, "Location directive outside a server block, ignored\n");
	}
	return;
}

void
f_try_target(char *target) {
	locations->type = TYPE_TRY_FILES;
	_try_target *tt = malloc(sizeof(_try_target));
	_try_target *prev = tryTargets;
	if (prev == NULL) {
		tryTargets = tt;
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
f_protocol(char *p) {
	if (p != NULL) {
		fprintf(stderr,"Duplicate protocol, ignored\n");
		return;
	}
	int len = strlen(p);
	if ((len == 5) 
		&& (strcmp(p, "https") == 0)) {
		protocol = PROTOCOL_HTTPS;
	} else if ((len == 5)
			&& (strcmp(p, "http2") == 0)) {
		protocol = PROTOCOL_HTTP2;
	} else if ((len == 4)
			&& (strcmp(p, "http") == 0)) {
		protocol = PROTOCOL_HTTP;
	} else {
		fprintf(stderr, "Unsupported protocol %s, ignored\n", p);
		return;
	}
	if (g.debug) {
		fprintf(stderr, "Proxy pass to %s protocol\n", p);
	}
	return;
}

// proxy_pass directive
void
f_proxy_pass(char *host, int port) {
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
	locations->passTo = passTo;
	locations->type = TYPE_PROXY_PASS;
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
	defaultLocation();
	defaultIndexFile();

	yyin = fopen(g.configFile, "r");
	if (yyin == NULL) {
		int e = errno;
		char buffer[BUFF_SIZE];
		snprintf(buffer, BUFF_SIZE, "%s: file open failed: %s\n", g.configFile, strerror(e));
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
		s->accessLog = g.accessLogs;
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
	openLogFiles();
}

/**
 * Check for a port defined as TLS for one server and non-TLS for another.
 * Return 1 = ok, 0 = not ok
 */
int
checkPorts(int portNum, int tlsFlag) {
	_server *s= g.servers;
	while(s) {
		_port *p = s->ports;
		while(p) {
			if ((p->portNum == portNum)
				&& (s->tls != tlsFlag)) {
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
		_port *p = server->ports;
		while(p) {
			if (checkPorts(p->portNum, s->tls) == 0) {
				fprintf(stderr, "HTTP and HTTPS on the same port not supported, port %d\n", p->portNum);
				return 0;
			}
		}
		s = s->next;
	}
	// unique port/tls combination
	return 1;
}
