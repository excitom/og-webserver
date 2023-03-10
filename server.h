// Max number of events to process in one gulp (but NOT max possible events)
#define EPOLL_ARRAY_SIZE   64

void parseArgs(int, char**);
int epollCreate();
int createBindAndListen(int);
void cleanup(int);
void doTrace (char, unsigned char*, int);
void doDebug (unsigned char*);
void processInput(int);
int sendData(int, unsigned char*, int);
int recvData(int, unsigned char*, int);
void getTimestamp(unsigned char *);
void sendErrorResponse( int, int, char *);
void handleGetVerb(int, char *);
void parseMimeTypes();

#define BUFF_SIZE 4096
#define TIME_BUF 256

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
	struct _mimeTypes *mimeTypes;
};
