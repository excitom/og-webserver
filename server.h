void parseArgs(int, char**);
void daemonize();
int epollCreate();
int createBindAndListen(int);
void cleanup(int);
void doTrace (char, unsigned char*, int);
void doDebug (unsigned char*);
#include <openssl/ssl.h>
void processInput(int, SSL*);
void configureContext(SSL_CTX*);
SSL_CTX *createContext();
void ShowCerts(SSL*);
int sendData(int, SSL*, unsigned char*, int);
int recvData(int, unsigned char*, int);
void getTimestamp(unsigned char*, int);
void sendErrorResponse(int, SSL*,int, char*, char*);
void handleGetVerb(int, SSL*, char*, char*);
void parseMimeTypes();
void accessLog(int, char*, int, char*, int);
void errorLog(int, char*, int, char*, char*);
void openLogFiles();
void getMimeType(char*, char*);
void showDirectoryListing(int, SSL*, char *);

#define BUFF_SIZE 4096
#define TIME_BUF 256

// timestamp formats
#define RESPONSE_FORMAT 0
#define LOG_FILE_FORMAT 1
#define LOG_RECORD_FORMAT 2

// mime types list
struct _mimeTypes {
	struct _mimeTypes *next;
	char *mimeType;
	char *extension;
};

// global variables
struct globalVars {
	int debug;
	int trace;
	int foreground;
	int dirList;
	int epollArraySize;
	int useTLS;
	unsigned short port;
	char *docRoot;
	char *configPath;
	char *logPath;
	int accessFd;
	int errorFd;
	struct _mimeTypes *mimeTypes;
};
