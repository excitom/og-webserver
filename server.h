// Max number of events to process in one gulp (but NOT max possible events)
#define EPOLL_ARRAY_SIZE   64

void parseArgs(int, char**);
void daemonize();
int epollCreate();
int createBindAndListen(int);
void cleanup(int);
void doTrace (char, unsigned char*, int);
void doDebug (unsigned char*);
void processInput(int);
int sendData(int, unsigned char*, int);
int recvData(int, unsigned char*, int);
void getTimestamp(unsigned char *, int);
void sendErrorResponse( int, int, char *);
void handleGetVerb(int, char *);
void parseMimeTypes();

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
	unsigned short port;
	char *docRoot;
	char *configPath;
	char *logPath;
	int accessFd;
	int errorFd;
	struct _mimeTypes *mimeTypes;
};
