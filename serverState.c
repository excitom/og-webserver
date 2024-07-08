/**
 * This is a collection of all the globally accessible state 
 * variables. Setters and getters are used to control access
 * rather than exposing the variables.
 *
 * Some of the objects are linked lists of objects, some are single values.
 *
 * Some variables can be changed from their default based on a config file
 * setting but otherwise the server does not support changing its configuration
 * other then killing and restarting the process.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>
#include "serverlist.h"
#include "mimeTypes.h"
#include "clients.h"

////////////////////////////////////////
// Debug mode?
static bool debug = false;
void
setDebug(const bool d) {
	debug = d;
}
bool
isDebug() {
	return debug;
}

////////////////////////////////////////
// Run server in the foreground? (not a daemon process)
static bool foreground = false;
void
setForeground(const bool f) {
	foreground = f;
}
bool
isForeground() {
	return foreground;
}

////////////////////////////////////////
// Is there a default server?
static bool defaultServer = true;
void
setDefaultServer(const bool d) {
	defaultServer = d;
}
bool
isDefaultServer() {
	return defaultServer;
}

////////////////////////////////////////
// Is network trace enabled?
static bool trace = false;
void
setTrace(const bool t) {
	trace = t;
}
bool
isTrace() {
	return trace;
}

////////////////////////////////////////
// Are server tokens enabled? (show the version number in response header)
static bool tokens = true;
void
setServerTokens(const bool t) {
	tokens = t;
}
bool
isServerTokensOn() {
	return tokens;
}

////////////////////////////////////////
// Test the configuration but don't start the server?
static bool testConfig = false;
void
setTestConfig(const bool t) {
	testConfig = t;
}
bool
isTestConfig() {
	return testConfig;
}

////////////////////////////////////////
// Show the server version but don't start the server?
static bool showVersion = false;
void
setShowVersion(const bool s) {
	showVersion = s;
}
bool
isShowVersion() {
	return showVersion;
}

////////////////////////////////////////
// Enables or disables the use of the TCP_NOPUSH socket option on FreeBSD
// or the TCP_CORK socket option on Linux.
// The options are enabled only when sendfile is used.
static bool tcpNoPush = false;
void
setTcpNoPush(const bool t) {
	tcpNoPush = t;
}
bool
isTcpNoPush() {
	return tcpNoPush;
}

////////////////////////////////////////
// Enables or disables the use of the sendfile system call.
static bool useSendFile = false;
void
setSendFile(const bool s) {
	useSendFile = s;
}
bool
isSendFile() {
	return useSendFile;
}

////////////////////////////////////////
// Keep track of the number of worker connections
static int workerConnections = 64;
void
setWorkerConnections(const int c) {
	workerConnections = c;
}
int
getWorkerConnections() {
	return workerConnections;
}

////////////////////////////////////////
// Keep track of the number of worker proccesses
static int workerProcesses = 1;
void
setWorkerProcesses(const int p) {
	workerProcesses = p;
}
int
getWorkerProcesses() {
	return workerProcesses;
}

////////////////////////////////////////
// The keepalive timeout value
static int keepaliveTimeout = 1;
void
setKeepaliveTimeout(const int t) {
	keepaliveTimeout = t;
}
int
getKeepaliveTimeout() {
	return keepaliveTimeout;
}

////////////////////////////////////////
// Signal to be sent to the lead server process
static char *signalName = NULL;
void
setSignalName(char *n) {
	if (signalName) {
		fprintf(stderr, "Duplicate setting of the signal name\n");
		exit(1);
	}
	signalName = n;
}
char *
getSignalName() {
	return signalName;
}

////////////////////////////////////////
// File which contains the lead process ID
static char *pidFile = NULL;
void
setPidFile(char *n) {
	// The default can be changed
	if (pidFile != NULL) {
		free(pidFile);
	}
	pidFile = n;
}
char *
getPidFile() {
	return pidFile;
}

////////////////////////////////////////
// Version number of the server
static char *version = NULL;
void
setVersion(char *v) {
	// The default can be changed
	if (version != NULL) {
		free(version);
	}
	version = v;
}
char *
getVersion() {
	if (isServerTokensOn()) {
		return version;
	} else {
		return "";
	}
}

////////////////////////////////////////
// user name of the server
static char *user = NULL;
void
setUser(char *u) {
	// The default can be changed
	if (user != NULL) {
		free(user);
	}
	user = u;
}
char *
getUser() {
	return user;
}

////////////////////////////////////////
// group name of the server
static char *group = NULL;
void
setGroup(char *g) {
	// The default can be changed
	if (group != NULL) {
		free(group);
	}
	group = g;
}
char *
getGroup() {
	return group;
}

////////////////////////////////////////
// Linked list of server configurations.
// The order matters so the list is kept first in, last out.
static _server *servers = NULL;
void
setServerList(_server *server) {
	_server *s = servers;
	if (s) {
		while(s->next) {
			s = s->next;
		}
		s->next = server;
	} else {
		servers = server;
	}
	server->next = NULL;
}
_server *
getServerList() {
	return servers;
}
//
// The first server in the list is the default server. If the config file
// specifies there shouldn't be a default server then pop the first one
// (the default) from the list.
//
_server *
popServer() {
	_server *s = servers;
	servers = s->next;
	if (s->accessLog) {
		free(s->accessLog);
	}
	if (s->errorLog) {
		free(s->errorLog);
	}
	free(s);
	return servers;
}

////////////////////////////////////////
// Linked list of upstream servers.
// The order matters so the list is kept first in, last out.
static _upstreams *upstreams = NULL;
void
setUpstreamList(_upstreams *upstream) {
	_upstreams *u = upstreams;
	if (u) {
		while(u->next) {
			u = u->next;
		}
		u->next = upstream;
	} else {
		upstreams = upstream;
	}
	upstream->next = NULL;
}
_upstreams *
getUpstreamList() {
	return upstreams;
}

////////////////////////////////////////
// Linked list of client connections.
// The order doesn't matter.
// Client connections come and go so this variable has a remove method.
// The client removed depends on the client's file descriptor.
//
// Since the TLS server uses threads we need a mutex here to serialize
// changes o the data structure.
//
static _clientConnection *clients = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void
setClientConnection(_clientConnection *client) {
	pthread_mutex_lock(&mutex);
	client->next = clients;
	clients = client;
	pthread_mutex_unlock(&mutex);
}
_clientConnection *
getClientConnection(int fd) {
	pthread_mutex_lock(&mutex);
	_clientConnection *c = clients;
	while(c) {
		if (c->fd == fd) {
			break;
		}
		c = c->next;
	}
	pthread_mutex_unlock(&mutex);
	return c;
}
_clientConnection *
removeClientConnection(int fd) {
	pthread_mutex_lock(&mutex);
	_clientConnection *c = clients;
	_clientConnection *prev = NULL;
	while(c) {
		if (c->fd == fd) {
			if (prev) {
				prev->next = c->next;
			} else {
				clients = c->next;
			}
			break;
		}
		prev = c;
		c = c->next;
	}
	pthread_mutex_unlock(&mutex);
	return c;
}

////////////////////////////////////////
// List of access log files
static _log_file *accessLog = NULL;
void
setAccessLog(_log_file *log) {
	_log_file * l = accessLog;
	if (l) {
		while (l->next) {
			l = l->next;
		}
		l->next = log;
	} else {
		accessLog = log;
	}
	log->next = NULL;
}
_log_file *
getDefaultAccessLog() {
	return accessLog;
}

////////////////////////////////////////
// List of error log files
static _log_file *errorLog = NULL;
void
setErrorLog(_log_file *log) {
	_log_file * l = errorLog;
	if (l) {
		while (l->next) {
			l = l->next;
		}
		l->next = log;
	} else {
		errorLog = log;
	}
	log->next = NULL;
}
_log_file *
getDefaultErrorLog() {
	return errorLog;
}

////////////////////////////////////////
// config file path
static char *configFile = NULL;
void
setConfigFile(char *p) {
	if (configFile) {
		fprintf(stderr, "Config file already set");
		exit(1);
	}
	configFile = p;
}
char *
getConfigFile() {
	return configFile;
}

static char *configDir = NULL;
void
setConfigDir(char *p) {
	if (configDir) {
		fprintf(stderr, "Config path already set");
		exit(1);
	}
	if (p[strlen(p)-1] != '/') {
		fprintf(stderr, "Config dir missing trailing path");
		exit(1);
	}
	configDir = p;
}
char *
getConfigDir() {
	return configDir;
}

////////////////////////////////////////
// List of mime types
static _mimeTypes *mimeTypes = NULL;
void
setMimeTypeList(_mimeTypes *mimeType) {
	_mimeTypes *m = mimeTypes;
	if (m) {
		while (m->next) {
			m = m->next;
		}
		m->next = mimeType;
	} else {
		mimeTypes = mimeType;
	}
	mimeType->next = NULL;
}
_mimeTypes *
getMimeTypeList() {
	return mimeTypes;
}

////////////////////////////////////////
// Default MIME type for responses
static char *defaultType = NULL;
void
setDefaultType(char *t) {
	if (defaultType != NULL) {
		free(defaultType);
	}
	defaultType = t;
}
char *
getDefaultType() {
	return defaultType;
}
