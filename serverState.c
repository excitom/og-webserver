/**
 * server state variables
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

////////////////////////////////////////
// debug mode?
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
// run server in the foreground?
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
// is there a default server?
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
// is network trace enabled?
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
// test the configuration but don't start the server?
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
// show the server version but don't start the server?
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
// Signal to be sent to the lead server process
static char *signalName = NULL;
void
setSignalName(char *n) {
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
	if (version != NULL) {
		free(version);
	}
	version = v;
}
char *
getVersion() {
	return version;
}

////////////////////////////////////////
// Linked list of server configurations
static _server *servers;
void
setServerList(char *s) {
	if (version != NULL) {
		free(version);
	}
	version = v;
}
char *
getServerList() {
	return version;
}

