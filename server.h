void parseArgs(int, char**);
void daemonize();
int epollCreate();
int createBindAndListen(int);
void cleanup(int);
void doTrace (char, char*, int);
void doDebug (char*);
#include <openssl/ssl.h>
void processInput(int, SSL*);
void configureContext(SSL_CTX*);
SSL_CTX *createContext();
void ShowCerts(SSL*);
int sendData(int, SSL*, char*, int);
int recvData(int, char*, int);
void getTimestamp(char*, int);
void sendErrorResponse(int, SSL*,int, char*, char*);
void handleGetVerb(int, SSL*, char*, char*);
void parseMimeTypes();
void parseConfig();
void checkConfig();
void accessLog(int, char*, int, char*, int);
void errorLog(int, char*, int, char*, char*);
void getMimeType(char*, char*);
void showDirectoryListing(int, SSL*, char *);
void server();
void sslServer();

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
	int foreground;
	int autoIndex;
	int workerConnections;
	int useTLS;
	int useSendfile;
	int keepaliveTimeout;
	unsigned short port;
	char *configPath;
	char *indexFile;
	char *docRoot;
	char *accessLog;
	char *errorLog;
	char *serverName;
	char *certFile;
	char *keyFile;
	char *pidFile;
	char *user;
	int accessFd;
	int errorFd;
	struct _mimeTypes *mimeTypes;
};
