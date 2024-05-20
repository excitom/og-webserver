/**
 * server state variables
 */
#include <stdbool.h>

////////////////////////////////////////
// debug mode?
bool debug = false;
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
bool foreground = false;
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
bool defaultServer = true;
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
bool trace = false;
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
bool testConfig = false;
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
bool showVersion = false;
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
bool tcpNoPush = false;
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
bool useSendFile = false;
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
bool workerConnections = 64;
void
setWorkerConnections(const int c) {
	workerConnections = c;
}
bool
getWorkerConnections() {
	return workerConnections;
}

////////////////////////////////////////
// Keep track of the number of worker proccesses
bool workerProcesses = 1;
void
setWorkerProcesses(const int p) {
	workerProcesses = p;
}
bool
getWorkerProcesses() {
	return workerProcesses;
}

