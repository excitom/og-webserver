#include "serverlist.h"
#include "clients.h"

void parseArgs(int, char**, char *);
void daemonize();
int epollCreate();
int createBindAndListen(int, int);
void cleanup(int);
void doTrace (char, char*, int);
void doDebug (char*);
#include <openssl/ssl.h>
void processInput(int, SSL*);
_clientConnection *queueClientConnection(int fd, struct sockaddr_in, SSL_CTX *ctx);
_clientConnection *getClient(int);
void configureContext(SSL_CTX*, int port);
SSL_CTX *createContext();
void ShowCerts(SSL*);
int sendData(int, SSL*, char*, int);
int recvData(int, char*, int);
void sendFile(int sockfd, int fd, SSL* ssl, size_t size);
void getTimestamp(char*, int);
void sendErrorResponse(int, SSL*,int, char*, char*);
void handleGetVerb(int, SSL*, _server*, _location *,char*, char*);
void parseMimeTypes();
void parseConfig();
void checkConfig();
void accessLog(int, char*, int, char*, int);
void errorLog(int, char*, int, char*, char*);
void getMimeType(char*, char*);
void showDirectoryListing(int, SSL*, char *, char *);
void server(int);
void tlsServer();
_location *getDocRoot(_server *, char *);
void handleProxyPass(int, char *, _location *);
int openDefaultIndexFile(_server *, char *);

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
	char *pidFile;
	char *user;
	char *group;
	char *signal;
	char *version;
	char *accessLog;
	char *errorLog;
	int accessFd;
	int errorFd;
	_server *servers;
	_clientConnection *clients;
	int portCount;
	struct _mimeTypes *mimeTypes;
};
