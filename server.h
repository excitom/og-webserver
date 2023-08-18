#include "clients.h"

void parseArgs(int, char**, char *);
void daemonize();
int epollCreate();
int createBindAndListen(int, int);
void cleanup(int);
void doTrace (char, char*, int);
void doDebug (char*);
#include <openssl/ssl.h>
void processInput(_request *);
_clientConnection *queueClientConnection(int fd, struct sockaddr_in, SSL_CTX *ctx);
_clientConnection *getClient(int);
void configureContext(SSL_CTX*, int port);
SSL_CTX *createContext();
void ShowCerts(SSL*);
int sendData(int, SSL*, char*, int);
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
void handleTryFiles(_request *);
int openDefaultIndexFile(_request *);
int pathExists(_request *, char *);
void serveFile(_request *);
FILE *expandIncludeFiles(char *);

#define FAIL    -1
#define BUFF_SIZE 4096
#define TIME_BUF 256

// timestamp formats
#define RESPONSE_FORMAT 0
#define LOG_FILE_FORMAT 1
#define LOG_RECORD_FORMAT 2

// mime types list
typedef struct _mimeTypes {
	struct _mimeTypes *next;
	char *mimeType;
	char *extension;
}_mimeTypes;

// global variables
struct globalVars {
	int debug;
	int trace;
	int testConfig;
	int foreground;
	int tcpNoPush;
	int sendFile;
	int workerConnections;
	int workerProcesses;
	int showVersion;
	int useSendfile;
	int keepaliveTimeout;
	char *configFile;
	char *configDir;
	char *pidFile;
	char *user;
	char *group;
	char *signal;
	char *version;
	char *defaultType;
	_log_file *accessLogs;
	_log_file *errorLogs;
	_server *servers;
	_clientConnection *clients;
	int portCount;
	struct _mimeTypes *mimeTypes;
};
