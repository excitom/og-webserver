#ifndef __SERVER
#define __SERVER
#include "clients.h"
#include <stdbool.h>
#include "mimeTypes.h"

void setDebug(bool);
bool isDebug();
void setForeground(bool);
bool isForeground();
void setDefaultServer(bool);
bool isDefaultServer();
void setTrace(bool);
bool isTrace();
void setServerTokens(bool);
bool isServerTokensOn();
void setTestConfig(bool);
bool isTestConfig();
void setShowVersion(bool);
bool isShowVersion();
void setTcpNoPush(bool);
bool isTcpNoPush();
void setSendFile(bool);
bool isSendFile();
void setWorkerConnections(int);
int getWorkerConnections();
void setKeepaliveTimeout(int);
int getKeepaliveTimeout();
void setWorkerProcesses(int);
int getWorkerProcesses();
void setSignalName(char *);
char *getSignalName();
void setPidFile(char *);
char *getPidFile();
void setVersion(char *);
char *getVersion();
void setUser(char *);
char *getUser();
void setGroup(char *);
char *getGroup();
void setServerList(_server *);
_server *getServerList();
_server *popServer();
void setClientConnection(_clientConnection *);
_clientConnection *getClientConnection(int);
_clientConnection *removeClientConnection(int);
void setAccessLog(_log_file *);
_log_file *getDefaultAccessLog();
void setErrorLog(_log_file *);
_log_file *getDefaultErrorLog();
void setConfigFile(char *);
char * getConfigFile();
void setConfigDir(char *);
char * getConfigDir();
void setMimeTypeList(_mimeTypes *);
_mimeTypes *getMimeTypeList();
void setDefaultType(char *);
char *getDefaultType();
void setUpstreamList(_upstreams *);
_upstreams *getUpstreamList();

void parseArgs(int, char**, const char *);
void daemonize();
int epollCreate();
int createBindAndListen(int, int);
void cleanup(int);
void doTrace (char, const char*, int);
void doDebug (char*);
#include <openssl/ssl.h>
void processInput(_request *);
_clientConnection *queueClientConnection(int, int, struct sockaddr_in, SSL_CTX*);
_clientConnection *getClient(int);
void configureContext(SSL_CTX*, int port);
SSL_CTX *createContext();
void ShowCerts(SSL*);
int sendData(int, SSL*, const char*, int);
int recvData(int, char*, int);
void sendFile(_request *, size_t size);
void getTimestamp(char*, int);
void sendErrorResponse(_request *,int, char*, char*);
void handleGetVerb(_request *);
void parseMimeTypes();
void parseConfig();
void checkConfig();
void accessLog(int, int, char*, int, char*, int);
void errorLog(int, int, char*, int, char*, char*);
void getMimeType(char*, char*);
void showDirectoryListing(_request *);
void server(int, int);
void tlsServer();
_location *getDocRoot(_server *, char *);
void handleProxyPass(_request *);
void handleFastCGIPass(_request *);
void handleTryFiles(_request *);
int getUpstreamServer(_request *);
int openDefaultIndexFile(_request *);
int pathExists(_request *, char *);
void serveFile(_request *);
FILE *expandIncludeFiles(char *);
void checkParameter(char *, char *);

#define FAIL    -1
#define BUFF_SIZE 4096
#define TIME_BUF 256

// timestamp formats
#define RESPONSE_FORMAT 0
#define LOG_FILE_FORMAT 1
#define LOG_RECORD_FORMAT 2

// global variables
struct globalVars {
	int useSendfile;
	int keepaliveTimeout;
	char *user;
	char *group;
	_log_file *errorLogs;
	_clientConnection *clients;
	struct _upstreams *upstreams;
};
#endif
