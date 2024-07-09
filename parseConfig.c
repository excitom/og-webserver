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
#include <math.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include "serverlist.h"
#include "server.h"
#include "parseConfig.h"

static _index_file *indexFiles = NULL;
static _server_name *serverNames = NULL;
static _port *ports = NULL;
static _location *locations = NULL;
static _try_target *tryTargets = NULL;
static _upstream *servers = NULL;
static char *certFile = NULL;
static char *keyFile = NULL;
static int autoIndex = 0;
static int protocol = PROTOCOL_UNSET;

/**
 * This is the interface to generated parser code from yacc/lex.
 */
extern FILE *yyin;
int yyparse (void);
char tempFile[] = "/tmp/ogws.conf";

// common error exit
void
errorExit(char *msg)
{
	fprintf(stderr, msg);
	fprintf(stderr, "Bad configuration, exiting\n");
	fprintf(stderr, "Examine the file %s to determine the error.\n", tempFile);
	exit(1);
}

void
parseConfig() {
	yyin = expandIncludeFiles((char *)&tempFile);
	defaultAccessLog();
	defaultErrorLog();
	defaultPort();
	defaultServerName();
	defaultLocation();
	defaultIndexFile();
	defaultType();
	// call the parser
	if (yyparse() != 0) {
		errorExit("Config file not parsed correctly.\n");
	}
	fclose(yyin);
	unlink((char *)&tempFile);
}

/**
 * Perform sanity checks on the configuration
 */
void
checkConfig()
{
	const char *pidFile = getPidFile();
	FILE *fp = fopen(pidFile, "r");
	if (fp == NULL) {
		perror("Can't open file");
		errorExit("pid log not valid\n");
	} else {
		fclose(fp);
	}

	checkServers();
	openLogFiles();
}

/**
 * Make sure there is at least one server name for a server
 */
void
checkServerNames(_server *s) {
	if (s->serverNames == NULL) {
		errorExit("missing server name\n");
	}
}

/**
 * Make sure each server has at least one index file
 */
void
checkIndexFiles(_server *s) {
	if (s->indexFiles == NULL) {
		errorExit("missing default index file\n");
	}
}

/**
 * Make sure each server has an access log
 */
void
checkAccessLog(_server *s) {
	if (s->accessLog == NULL) {
		// use default
		s->accessLog = getDefaultAccessLog();
		if (s->accessLog == NULL) {
			errorExit("missing default access log\n");
		}
	}
}

/**
 * Make sure each server has an error log
 */
void
checkErrorLog(_server *s) {
	if (s->errorLog == NULL) {
		// use default
		s->errorLog = getDefaultErrorLog();
		if (s->errorLog == NULL) {
			errorExit("missing default error log\n");
		}
	}
}

/**
 * For SSL/TLS servers,
 * make sure there is a cert file.
 */
void
checkCertFile(_server *s) {
	if (access(s->certFile, R_OK) == -1) {
		fprintf(stderr, "%s: ", s->certFile);
		perror("can't open file");
		errorExit("certificate file not valid\n");
	}
}

/**
 * For SSL/TLS servers,
 * make sure there is a key file.
 */
void
checkKeyFile(_server *s) {
	if (access(s->keyFile, R_OK) == -1) {
		fprintf(stderr, "%s: ", s->keyFile);
		perror("can't open file");
		errorExit("key file not valid");
	}
}

/**
 * Loop through the servers and perform integrity checks
 *
 * Note: By default there is a default server that listens for
 * any host name on port 8080. This can be overridden by a command
 * line argument.
 */
void
checkServers() {
	_server *s = getServerList();
	if (!isDefaultServer()) {
		// The first server in the list is the default server.
		// If the configuration doesn't want a default server,
		// then remove the first (default) server.
		s = popServer();
	}
	while(s) {
		if (!portOk(s)) {
			errorExit("Invalid port configuration\n");
		}
		checkDocRoots(s);
		checkServerNames(s);
		checkIndexFiles(s);
		checkAccessLog(s);
		checkErrorLog(s);
		if (s->tls) {
			checkCertFile(s);
			checkKeyFile(s);
		}
		s = s->next;
	}
}

/**
 * Check for a port defined as TLS for one server and non-TLS for another.
 * Return 1 = ok, 0 = not ok
 */
int
checkPorts(int portNum, int tlsFlag) {
	_server *s= getServerList();
	while(s) {
		_port *p = s->ports;
		while(p) {
			if ((p->portNum == portNum)
				&& (p->tls != tlsFlag)) {
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
	_server *s = getServerList();
	while(s) {
		_port *p = server->ports;
		while(p) {
			if (checkPorts(p->portNum, p->tls) == 0) {
				fprintf(stderr, "HTTP and HTTPS on the same port not supported, port %d\n", p->portNum);
				return 0;
			}
			p = p->next;
		}
		s = s->next;
	}
	// unique port/tls combination
	return 1;
}

/**
 * check if a `proxy_pass` target is an upstream group
 */
_upstreams *
isUpstreamGroup(char *host)
{
	_upstreams *group = getUpstreamList();
	while(group) {
		if (strlen(host) == strlen(group->name) &&
				strcmp(host, group->name) == 0) {
			return group;
		}
		group = group->next;
	}
	return NULL;
}

/** 
 * Set up a `proxy_pass` to an upstream group
 */
void
proxyPassToUpstreamGroup(int type, char *host, _upstreams *group)
{
	_location *defLoc = locations;
	// get the default location
	while (defLoc->next) {
		defLoc = defLoc->next;
	}
	// if there is a pending location in addition to the default,
	// update the pending location.
	// set defaults
	if (locations != defLoc) {
		locations->type |= type;
		if (locations->group) {
			free(locations->group);
			doDebug("Duplicate upstream group");
		}
		locations->group = group;
		if (isDebug()) {
			fprintf(stderr,"Updated upstream group %s\n", host);
		}
	} else {
		_location *loc = (_location *)calloc(1, sizeof(_location));
		memcpy(loc, defLoc, sizeof(_location));
		loc->next = locations;
		locations = loc;
		loc->group = group;
		loc->type |= TYPE_UPSTREAM_GROUP;
		if (isDebug()) {
			fprintf(stderr,"New upstream group %s\n", host);
		}
	}
	// update all the servers in the upstream group
	_upstream *up = group->servers;
	while (up) {
		struct sockaddr_in *passTo = (struct sockaddr_in *)calloc(1, sizeof(struct sockaddr_in));
		struct hostent *hostName = gethostbyname(up->host);
		if (hostName == (struct hostent *)0) {
			fprintf(stderr, "%s ", up->host);
			errorExit("gethostbyname failed\n");
		}
		passTo->sin_family = AF_INET;
		passTo->sin_port = htons(up->port);
		passTo->sin_addr.s_addr = *((unsigned long*)hostName->h_addr);
		up->passTo = passTo;
		up = up->next;
	}
}

/** 
 * Set up a `proxy_pass` to a single host
 */
void
proxyPassToHost(int type, char * host, int port)
{
	struct sockaddr_in *passTo = (struct sockaddr_in *)calloc(1, sizeof(struct sockaddr_in));
	struct hostent *hostName = gethostbyname(host);
	if (hostName == (struct hostent *)0) {
		fprintf(stderr, "%s ", host);
		errorExit("gethostbyname failed\n");
	}
	passTo->sin_family = AF_INET;
	passTo->sin_port = htons(port);
	passTo->sin_addr.s_addr = *((unsigned long*)hostName->h_addr);

	_location *defLoc = locations;
	// get the default location
	while (defLoc->next) {
		defLoc = defLoc->next;
	}
	// if there is a pending location in addition to the default,
	// update the pending location.
	// set defaults
	if (locations != defLoc) {
		locations->type |= type;
		if (locations->passTo) {
			free(locations->passTo);
			doDebug("Duplicate proxy pass");
		}
		locations->passTo = passTo;
		if (isDebug()) {
			fprintf(stderr,"Updated proxy pass host %s\n", host);
		}
	} else {
		_location *loc = (_location *)calloc(1, sizeof(_location));
		memcpy(loc, defLoc, sizeof(_location));
		loc->next = locations;
		locations = loc;
		loc->passTo = passTo;
		loc->type |= TYPE_PROXY_PASS;
		if (isDebug()) {
			fprintf(stderr,"New proxy pass host %s\n", host);
		}
	}
}

/**
 * Define a default access log
 */
void
defaultAccessLog()
{
	_log_file *log = (_log_file *)calloc(1, sizeof(_log_file));
	char path[] = "/var/log/ogws/access.log";
	log->path = (char *)calloc(1, strlen(path)+1);
	strcpy(log->path, path);
	log->fd = -1;
	log->next = NULL;
	setAccessLog(log);
}

/**
 * Define a default error log
 */
void
defaultErrorLog()
{
	_log_file *log = (_log_file *)calloc(1, sizeof(_log_file));
	char path[] = "/var/log/ogws/error.log";
	log->path = (char *)calloc(1, strlen(path)+1);
	strcpy(log->path, path);
	log->fd = -1;
	log->next = NULL;
	setErrorLog(log);
}

/**
 * Define a default listening port
 */
void
defaultPort()
{
	_port *p = (_port *)calloc(1, sizeof(_port));
	p->portNum = 8080;
	p->next = NULL;
	ports = p;
}

/**
 * Define a default server name
 */
void
defaultServerName()
{
	_server_name *sn = (_server_name *)calloc(1, sizeof(_server_name));
	sn->serverName = "*";
	sn->next = NULL;
	serverNames = sn;
}

/**
 * Define a default document root which matches all locations
 */
void
defaultLocation()
{
	_location *loc = (_location *)calloc(1, sizeof(_location));
	loc->type = TYPE_DOC_ROOT;
	loc->matchType = PREFIX_MATCH;
	loc->match = "/";
	loc->root = "/var/www/ogws/html";
	loc->tryTarget = NULL;
	loc->passTo = NULL;	
	loc->group = NULL;	
	loc->expires = 0;
	loc->next = locations;
	locations = loc;
	return;
}

/**
 * Set the default MIME type for responses
 */
void
defaultType() {
	char d[] = "text/plain";
	char *dt = (char *)calloc(1, strlen(d)+1);
	strcpy(dt, d);
	setDefaultType(dt);
}

/**
 * Define a default index file
 */
void 
defaultIndexFile()
{
	_index_file *index = (_index_file *)calloc(1, sizeof(_index_file));
	index->indexFile = "index.html";
	index->next = NULL;
	indexFiles = index;
}

/**
 * Multiple servers may share the same log file(s). Share the file
 * descriptor in this case rather than re-openeing the file.
 */
int
pathAlreadyOpened(const char *path, _log_file *list)
{
	_log_file *log = list;
	while(log) {
		if ((strlen(path) == strlen(log->path))
			&& (strcmp(path, log->path) == 0)) {
			if (log->fd != -1) {
				return log->fd;
			}
		}
		log = log->next;
	}
	return -1;
}

/**
 * Open all the log files (access and error)
 */
void
openLogFiles()
{
	// list of one or more access logs starting with the default
	_log_file *log = getDefaultAccessLog();	
	while(log) {
		log->fd = pathAlreadyOpened(log->path, getDefaultAccessLog());
		if (log->fd == -1) {
			log->fd = open(log->path, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
		}
		if (log->fd == -1) {
			fprintf(stderr, "%s: ", log->path);
			perror("can't open file");
			errorExit("access log not valid\n");
		}
		log = log->next;
	}
	// list of one or more error logs starting with the default
	log = getDefaultErrorLog();	
	while(log) {
		log->fd = pathAlreadyOpened(log->path, getDefaultErrorLog());
		if (log->fd == -1) {
			log->fd = open(log->path, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
		}
		if (log->fd == -1) {
			fprintf(stderr, "%s: ", log->path);
			perror("can't open file");
			errorExit("error log not valid\n");
		}
		log = log->next;
	}
}

/**
 * Check sanity of the configuration
 */
void
checkDocRoots(_server *s)
{
	for(_location *loc = s->locations; loc != NULL; loc = loc->next) {
		if (loc->type & TYPE_PROXY_PASS) {
			if (!loc->passTo) {
				errorExit("Proxy pass missing\n");
			}
		}
		else if(loc->type & TYPE_DOC_ROOT) {
			if (access(loc->root, R_OK) == -1) {
				fprintf(stderr, "%s: ", loc->root);
				errorExit("doc root not valid\n");
			}
		}
		else if (loc->type & TYPE_TRY_FILES) {
			if (!loc->tryTarget) {
				errorExit("Try directive incomplete\n");
			}
		}
		else {
			fprintf(stderr, "%d: ", loc->type);
			errorExit("Unknown location type\n");
		}
	}
}

/**
 * The following `f_` functions implement the config file keywords
 * The functions are called from the lex/yacc generated code as the 
 * config file is parsed.
 */

// specify the pid file location
// Syntax:	pid file;
// Default:pid /etc/ogws/ogws.pid;
// Context:	main
void
f_pid(char *pidFile) {
	setPidFile(pidFile);
	if (isDebug()) {
		fprintf(stderr,"PID file location:  %s\n", pidFile);
	}
}

// Include a file. Note: This should never be called since a pre-processing
// step expands all the include files.
// Syntax:	include file | mask;
// Default:	—
// Context:	any
void
f_include(char *path) {
	if (isDebug()) {
		fprintf(stderr, "Include file ignored %s\n", path);
	}
}

// set the default MIME type for responses
// Syntax:	default_type mime-type;
// Default: default_type text/plain;
// Context:	http, server, location
void
f_default_type(char *type) {
	setDefaultType(type);
}

// specify option for network trace
void
f_trace(bool trace) {
	setTrace(trace);
	if (isTrace()) {
		fprintf(stderr,"Show network trace ON\n");
	} else {
		fprintf(stderr,"Show network trace OFF\n");
	}
}

// specify if directory listing should be allowed
// Syntax:	autoindex on | off;
// Default: autoindex off;
// Context:	http, server, location

void
f_autoindex(int flag) {
	autoIndex = flag;
	if (isDebug()) {
		fprintf(stderr,"Auto index %s\n", autoIndex ? "On" : "Off");
	}
	return;
}

// specify if the `sendfile` system call should be used
// Syntax:	sendfile on | off;
// Default: sendfile off;
// Context:	http, server, location, if in location
void
f_sendfile(bool sendFile) {
	setSendFile(sendFile);
	if (isDebug()) {
		if (isSendFile()) {
			fprintf(stderr,"Use sendfile system call ON\n");
		} else {
			fprintf(stderr,"Use sendfile system call OFF\n");
		}
	}
}

// specify if the `tcp_nopush` option
// Syntax:	tcp_nopush on | off;
// Default:	tcp_nopush off;
// Context:	http, server, location
void
f_tcpnopush(bool tcpNoPush) {
	setTcpNoPush(tcpNoPush);
	if (isDebug()) {
		if (isTcpNoPush()) {
			fprintf(stderr,"Use `tcp_nopush` option ON\n");
		} else {
			fprintf(stderr,"Use `tcp_nopush` option OFF\n");
		}
	}
}

// specify the user name to be used for the server process
// Syntax:	user user [group];
// Default: user nobody nobody;
// Context:	main
void
f_user(char *user, char *group) {
	setUser(user);
	if (isDebug()) {
		fprintf(stderr,"Run as user name %s\n", user);
	}
	if (group) {
		setGroup(group);
		if (isDebug()) {
			fprintf(stderr,"Run as group name %s\n", group);
		}
	}
}

// process the server section
void
f_server() {
	_server *server = (_server *)calloc(1, sizeof(_server));
	setServerList(server);
	_server_name *sn = serverNames;
	if (!sn) {
		errorExit("No server name\n");
	}
	// for server names, ports, index file names, locations:
	// 	  if there is only one item in the list, it is the default
	// 		copy it to the server
	// 	  else
	// 	  	move the list to the server, excluding the default
	if (!sn->next) {
		_server_name *copysn = (_server_name *)calloc(1, sizeof(_server_name));
		memcpy(copysn, sn, sizeof(_server_name));
		server->serverNames = copysn;
	} else {
		while(sn) {
			if (!sn->next) {
				// skip the default
				break;
			}
			serverNames = sn->next;
			sn->next = server->serverNames;
			server->serverNames = sn;
			sn = serverNames;
		}
	}
	_index_file *ifn = indexFiles;
	if (!ifn->next) {
		_index_file *copyIfn = (_index_file *)calloc(1, sizeof(_index_file));
		memcpy(copyIfn, ifn, sizeof(_index_file));
		server->indexFiles = copyIfn;
	} else {
		while(ifn) {
			if (!ifn->next) {
				// skip the default
				break;
			}
			indexFiles = ifn->next;
			ifn->next = server->indexFiles;
			server->indexFiles = ifn;
			ifn = indexFiles;
		}
	}
	_port *port = ports;
	if (!port->next) {
		_port *copyPort = (_port *)calloc(1, sizeof(_port));
		memcpy(copyPort, port, sizeof(_port));
		server->ports = copyPort;
	} else {
		while(port) {
			if (!port->next) {
				// skip the default
				break;
			}
			ports = port->next;
			port->next = server->ports;
			server->ports = port;
			port = ports;
		}
	}
	_location *loc = locations;
	char *defRoot = NULL;
	if (!loc->next) {
		_location *copyLoc = (_location *)calloc(1, sizeof(_location));
		memcpy(copyLoc, loc, sizeof(_location));
		server->locations = copyLoc;
		defRoot = copyLoc->root;
	} else {
		while(loc) {
			if (!loc->next) {
				// skip the default
				break;
			}
			if (!defRoot) {
				defRoot = loc->root;
			}
			// preserve the order of the locations
			if (server->locations) {
				_location *prev = server->locations;
				while(prev->next) {
					prev = prev->next;
				}
				prev->next = loc;
			} else {
				server->locations = loc;
			}
			locations = loc->next;
			loc->next = NULL;
			loc = locations;
		}
	}
	// propagate default doc root to all locations that need it
	loc = server->locations;
	while(loc) {
		if (!loc->root) {
			loc->root = defRoot;
		}
		loc = loc->next;
	}
	server->autoIndex = autoIndex;
	server->certFile = certFile;
	server->keyFile = keyFile;
	// reset defaults
	autoIndex = 0;
	certFile = NULL;
	keyFile = NULL;
	// pick the top of the stack but don't pop
	server->accessLog = getDefaultAccessLog();
	server->errorLog = getDefaultErrorLog();
	return;
}

// process the http section
void
f_http() {
	if (!getServerList()) {
		if (isDebug()) {
			doDebug("Missing server block, using default\n");
		}
	}
	// create a default server, even if there are servers, in case one of
	// them didn't define all the defaults.
	_server *server = (_server *)calloc(1, sizeof(_server));
	setServerList(server);
	_server_name *sn = serverNames;
	server->serverNames = sn;
	_index_file *i = indexFiles;
	server->indexFiles = i;
	_port *port = ports;
	server->ports = port;
	_location *loc = locations;
	server->locations = loc;
	server->autoIndex = 0;
	server->certFile = certFile;
	server->keyFile = keyFile;
	server->accessLog = getDefaultAccessLog();
	server->errorLog = getDefaultErrorLog();
	return;
}

// document root
// Syntax:	root path;
// Default: root html;
// Context:	http, server, location, if in location
void
f_root(char *root) {
	_location *defLoc = locations;
	// get the default location
	while (defLoc->next) {
		defLoc = defLoc->next;
	}
	// if there is a pending location in addition to the default,
	// update the pending location.
	if (locations != defLoc) {
		locations->type |= TYPE_DOC_ROOT;
		if (locations->root) {
			free(locations->root);
			doDebug("Duplicate doc root");
		}
		locations->root = root;
		if (isDebug()) {
			fprintf(stderr,"Updated Document root path %s\n", locations->root);
		}
	} else {
		_location *loc = (_location *)calloc(1, sizeof(_location));
		memcpy(loc, defLoc, sizeof(_location));
		loc->next = locations;
		locations = loc;
		loc->root = root;
		loc->type |= TYPE_DOC_ROOT;
		if (isDebug()) {
			fprintf(stderr,"New Document root path %s\n", loc->root);
		}
	}
	return;
}

// expires directive
// Syntax:	expires [modified] time;
//          expires epoch | max | off;
// Default: expires off;
// Context:	http, server, location, if in location
void
f_expires(char *expires) {
	if (!locations) {
		fprintf(stderr, "Expires directive with no Location, ignored\n");
		free(expires);
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
				fprintf(stderr, "Unknown unit, ignored\n");
			}
			expires[len-1] = '\0';
			locations->expires = atoi(expires) * mult;
		}
	}
	if (isDebug()) {
		fprintf(stderr,"Expires directive %d\n", locations->expires);
	}
	free(expires);
	return;
}

// server name
// Syntax:	server_name name ...;
// Default:	 server_name "";
// Context:	server
int dupName(char *name) {
	_server_name *sn = serverNames;
	while(sn) {
		if (!sn->next) {
			return 0;	// skip the default
		}
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
	if (dupName(serverName)) {
		// duplicate name, ignored
		free(serverName);
	} else {
		_server_name *sn = (_server_name *)calloc(1, sizeof(_server_name));
		sn->serverName = serverName;
		sn->type = type;
		sn->next = serverNames;
		serverNames = sn;
	}
	if (isDebug()) {
		fprintf(stderr,"server name: %s\n", serverName);
	}
	return;
}

// Syntax:	server_tokens on | off | build | string;
// Default:	server_tokens on;
// Context:	http, server, location
void
f_server_tokens(bool flag) {
	setServerTokens(flag);
	if (isServerTokensOn()) {
		fprintf(stderr,"Show server tokens ON\n");
	} else {
		fprintf(stderr,"Show server tokens OFF\n");
	}
}

// index file name
// Syntax:	index file ...;
// Default: index index.html;
// Context:	http, server, location
void
f_indexFile(char *indexFile) {
	_index_file *i = (_index_file *)calloc(1, sizeof(_index_file));
	i->next = indexFiles;
	indexFiles = i;
	i->indexFile = indexFile;
	if (isDebug()) {
		fprintf(stderr, "Index File name: %s\n", i->indexFile);
	}
	return;
}

// error log file path
// Syntax:	error_log file [level];
// Default: error_log logs/error.log error;
// Context:	main, http, mail, stream, server, location
void
f_error_log(char *path) {
	_log_file *log = (_log_file *)calloc(1, sizeof(_log_file));
	log->path = path;
	log->fd = -1;
	setErrorLog(log);
	if (isDebug()) {
		fprintf(stderr,"Error log file path %s\n", log->path);
	}
	return;
}

// access log file path
// Syntax:	access_log path [format [buffer=size] [gzip[=level]] [flush=time] [if=condition]];
//           access_log off;
// Default: access_log logs/access.log combined;
// Context:	http, server, location, if in location, limit_except
void
f_access_log(char *path, int type) {
	_log_file *log = (_log_file *)calloc(1, sizeof(_log_file));
	log->path = path;
	log->type = type;
	log->fd = -1;
	setAccessLog(log);
	if (isDebug()) {
		fprintf(stderr,"Access log file path %s\n", log->path);
	}
	if (type) {
		fprintf(stderr, "Access log MAIN ignored\n");
	}
	return;
}

// SSL parameter for DHE ciphers
// Syntax:	ssl_dhparam file;
// Default:	—
// Context:	http, server
void
f_ssl_dhparam(char *path) {
	fprintf(stderr, "SSL DH parameter %s NOT YET IMPLEMENTED\n", path);
	return;
}

// Syntax:	ssl_protocols [SSLv2] [SSLv3] [TLSv1] [TLSv1.1] [TLSv1.2] [TLSv1.3];
// Default:	 ssl_protocols TLSv1 TLSv1.1 TLSv1.2 TLSv1.3;
// Context:	http, server
void
f_ssl_protocol(char *name) {
	fprintf(stderr, "SSL Protocol Name %s NOT YET IMPLEMENTED\n", name);
}

// Syntax:	ssl_ciphers ciphers;
// Default:	ssl_ciphers HIGH:!aNULL:!MD5;
// Context:	http, server
void
f_ssl_ciphers(char *name) {
	fprintf(stderr, "SSL Ciphers %s NOT YET IMPLEMENTED\n", name);
}

// SSL session tickets
// Syntax:	ssl_session_tickets on | off;
// Default: ssl_session_tickets on;
// Context:	http, server
void
f_ssl_session_tickets(bool flag) {
	if (flag) {
		fprintf(stderr, "Set ssl session tickets ON (NOT YET IMPLEMENTED)\n");
	} else {
		fprintf(stderr, "Set ssl session tickets OFF (NOT YET IMPLEMENTED)\n");
	}
}

// SSL server ciphers
// Syntax:	ssl_prefer_server_ciphers on | off;
// Default: ssl_prefer_server_ciphers off;
// Context:	http, server
void
f_ssl_prefer_server_ciphers(bool flag) {
	if (flag) {
		fprintf(stderr, "Set ssl prefer server ciphers ON (NOT YET IMPLEMENTED)\n");
	} else {
		fprintf(stderr, "Set ssl prefer server ciphers OFF (NOT YET IMPLEMENTED)\n");
	}
}

// SSL session timeout
// Syntax:	ssl_session_timeout time;
// Default: ssl_session_timeout 5m;
// Context:	http, server
void
f_ssl_session_timeout(char *units) {
	bool valid = true;
	int val = 0;
	int mult = 1;
	const int l = strlen(units);
	char c = units[l-1];
	switch(c) {
		case 's':
			mult = 1;
			break;
		case 'm':
			mult = 60;
			break;
		case 'h':
			mult = 60*60;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			mult = 1;
			break;
		default:
			valid = false;
	}
	if (!valid) {
		fprintf(stderr, "SSL session timeout: UNRECOGNIED PARAMETER %s\n", units);
	} else {
		val = atoi(units) * mult;
		fprintf(stderr, "SSL session timeout: %d (NOT YET IMPLEMENTED)\n", val);
	}
	return;
}
// the parameter is passed as an integer rather than with a UNITS suffix
void
f_ssl_session_timeout_num(int val) {
	fprintf(stderr, "SSL session timeout: %d (NOT YET IMPLEMENTED)\n", val);
	return;
}

// SSL session cache parameters
// Syntax:	ssl_session_cache off | none | [builtin[:size]] [shared:name:size];
// Default: ssl_session_cache none;
// Context:	http, server
bool
check_ssl_session_cache(char *spec) {
	bool valid = false;
	if (strncmp(spec, "builtin", strlen("builtin")) == 0) {
		fprintf(stderr, "SSL internal session cache will be used (NOT YET IMPLEMENTED)\n");
		valid = true;
		char *s = strchr(spec, ':');
		if (s && (strlen(s) > 1)) {
			s++;
			fprintf(stderr, "SSL internal session cache size: %s\n", s);
		}
	}
	if (strncmp(spec, "shared", strlen("shared")) == 0) {
		fprintf(stderr, "SSL shared session cache will be used (NOT YET IMPLEMENTED)\n");
		valid = true;
		char *s = strchr(spec, ':');
		if (!s) {
			fprintf(stderr, "INVALID SSL shared session cache size: %s\n", spec);
			valid = false;
		} else {
			s++;
			char *sz = strchr(s, ':');
			if (!s) {
				fprintf(stderr, "INVALID SSL shared session cache size: %s\n", spec);
				valid = false;
			} else {
				sz++;
				fprintf(stderr, "SSL shared session cache size: %s\n", sz);
			}
		}
	}
	return valid;
}
void
f_ssl_session_cache(char *spec, char *spec2) {
	bool valid = check_ssl_session_cache(spec);
	if (!valid) {
		fprintf(stderr, "SSL shared session cache: UNRECOGNIED PARAMETER %s\n", spec);
	}
	if (spec2) {
		valid = check_ssl_session_cache(spec2);
		if (!valid) {
			fprintf(stderr, "Second SSL shared session cache: UNRECOGNIED PARAMETER %s\n", spec2);
		}
	}
	return;
}
void
f_ssl_session_cache_off(char *spec) {
	bool valid = false;
	if (strcmp(spec, "off") == 0) {
		fprintf(stderr, "SSL session cache will be disabled (NOT YET IMPLEMENTED)\n");
		valid = true;
	}
	if (strcmp(spec, "none") == 0) {
		fprintf(stderr, "SSL session cache will be disabled (NOT YET IMPLEMENTED)\n");
		valid = true;
	}
	if (!valid) {
		fprintf(stderr, "SSL shared session cache: UNRECOGNIED PARAMETER %s\n", spec);
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
	if (isDebug()) {
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
	if (isDebug()) {
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
		if (!p->next) {
			return 0;	// skip the default
		}
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
			_port *p = (_port *)calloc(1, sizeof(_port));
			p->portNum = portNum;
			p->tls = 0;
			p->next = ports;
			ports = p;
		}
	}
	if (name != NULL) {
		if (dupName(name)) {
			// duplicate server inored
			free(name);
		} else {
			_server_name *sn = (_server_name *)calloc(1, sizeof(_server_name));
			sn->serverName = name;
			sn->next = serverNames;
			serverNames = sn;
		}
	}
	return;
}

void
f_tls() {
	ports->tls = 1;
}

// location directive
void
f_location(int matchType, char *match) {
	_location *loc = locations;
	loc->matchType = matchType;
	loc->match = match;
	loc->protocol = protocol;
	if (!loc->root) {
		if (!loc->next || ! loc->next->root) {
			errorExit("Missing default location\n");
		}
		loc->root = loc->next->root;
	}
	protocol = PROTOCOL_UNSET;
	return;
}

// upstream directive
// note: the `next` pointer chains the upstreams together.
// the `currentServer` pointer is the next upstream to get traffic.
void
f_upstreams(char *name) {
	_upstreams *up = (_upstreams *)calloc(1, sizeof(_upstreams));
	setUpstreamList(up);
	up->name = name;
	up->servers = up->currentServer = servers;
}

// upstream server component
void
f_upstream(char *host, int port, int weight) {
	int w = weight;
	while (w-- > 0) {
		_upstream *server = (_upstream *)calloc(1, sizeof(_upstream));
		server->next = servers;
		servers = server;
		server->host = host;
		server->port = port;
		server->weight = weight;
	}
}

void
f_try_target(char *target) {
	_try_target *tt = calloc(1, sizeof(_try_target));
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
	_location *defLoc = locations;
	// get the default location
	while (defLoc->next) {
		defLoc = defLoc->next;
	}
	// if there is a pending location in addition to the default,
	// update the pending location.
	// set defaults
	if (locations != defLoc) {
		locations->type |= TYPE_TRY_FILES;
		locations->matchType = UNSET_MATCH;
		locations->tryTarget = tryTargets;
		tryTargets = NULL;
		if (isDebug()) {
			fprintf(stderr,"Updated try files\n");
		}
	} else {
		_location *loc = (_location *)calloc(1, sizeof(_location));
		memcpy(loc, defLoc, sizeof(_location));
		loc->next = locations;
		locations = loc;
		loc->type |= TYPE_TRY_FILES;
		loc->matchType = UNSET_MATCH;
		loc->tryTarget = tryTargets;
		tryTargets = NULL;
		if (isDebug()) {
			fprintf(stderr,"New try files\n");
		}
	}
}

// proxy_pass protocol
void
f_protocol(char *p) {
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
	if (isDebug()) {
		fprintf(stderr, "Proxy pass to %s protocol\n", p);
	}
	return;
}

// proxy_pass directive
// note: proxy_pass and fastcgi_pass are handled basically the same.
// a flag is passed at runtime to indicate the difference.
void
f_proxy_pass(char *host, int port) {
	_upstreams *group = isUpstreamGroup(host);
	if (group) {
		proxyPassToUpstreamGroup(TYPE_PROXY_PASS, host, group);
	} else {
		proxyPassToHost(TYPE_PROXY_PASS, host, port);
	}
	free(host);		// no longer needed
	return;
}
//
// fastcgi_pass directive
void
f_fastcgi_pass(char *host, int port) {
	_upstreams *group = isUpstreamGroup(host);
	if (group) {
		proxyPassToUpstreamGroup(TYPE_FASTCGI_PASS, host, group);
	} else {
		proxyPassToHost(TYPE_FASTCGI_PASS, host, port);
	}
	free(host);		// no longer needed
	return;
}

// keepalive timeout
void
f_keepalive_timeout(int timeout) {
	setKeepaliveTimeout(timeout);
	if (isDebug()) {
		fprintf(stderr,"Keepalive timeout: %d\n", timeout);
	}
}

// fastcgi index, param, and split path info - unimplemented for now,
// until the fast CGI API is implemented.
void
f_fastcgi_index(char *file) {
	fprintf(stderr, "fastcgi_index file %s, unimplemented, ignored\n", file);
	free(file);
}
// Syntax:	fastcgi_param parameter value [if_not_empty];
// Default:	—
// Context:	http, server, location
void
f_fastcgi_param(char *name, char *value, char *value2) {
	if (value2) {
		size_t len = strlen(value)+strlen(value2)+1;
		char *v = malloc(len);
		strcpy(v, value);
		strcat(v, value2);
		free(value);
		free(value2);
		value = v;
	}
	checkParameter(name, value);
}
void
f_fastcgi_num_param(char *name, int value) {
	size_t len = (int)((ceil(log10(value))+1)*sizeof(char));
	char *str = malloc(len);
	sprintf(str, "%d", value);
	checkParameter(name, str);
}
void
f_fastcgi_split_path_info(char *regex) {
	fprintf(stderr, "fastcgi_split_path_info using regex %s, unimplemented, ignored\n", regex);
	free(regex);
}

// max worker processes
void
f_workerProcesses(int workerProcesses) {
	setWorkerProcesses(workerProcesses);
	if (isDebug()) {
		fprintf(stderr,"Max worker processes %d\n", getWorkerProcesses());
	}
}

// max worker connections
void
f_workerConnections(int workerConnections) {
	setWorkerConnections(workerConnections);
	if (isDebug()) {
		fprintf(stderr,"Max worker connections %d\n", getWorkerConnections());
	}
}

// config file parsed successfully
void
f_config_complete() {
	checkConfig();
	if (isDebug()) {
		fprintf(stderr, "Config file parsing complete!\n");
	}
}
