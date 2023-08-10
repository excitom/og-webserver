/**
 * Define the datastructures for maintaining server (virtual host)
 * configurations.
 */
#include <openssl/ssl.h>

typedef struct _log_file {
	struct _log_file *next;
	char *path;
	int fd;
	int type;
} _log_file;

#define TYPE_UNSET -1
#define TYPE_PROXY_PASS 0
#define TYPE_DOC_ROOT 1
#define TYPE_TRY_FILES 2
#define UNSET_MATCH -1
#define EQUAL_MATCH 0
#define REGEX_MATCH 1
#define PREFIX_MATCH 2
#define PROTOCOL_UNSET -1
#define PROTOCOL_HTTP 0
#define PROTOCOL_HTTPS 1
#define PROTOCOL_HTTP2 2
typedef struct _try_target {
	struct _try_target *next;
	char *target;
} _try_target;

typedef struct _location {
	struct _location *next;
	int type;
	int matchType;
	char *match;
	char *root;
	int protocol;
	_try_target *try_target;
	struct sockaddr_in *passTo;		// for proxy_pass locations
	int expires;
}_location;

#define SERVER_NAME_EXACT 0
#define SERVER_NAME_WILDCARD_PREFIX 1
#define SERVER_NAME_WILDCARD_SUFFIX 2
typedef struct _server_name {
	struct _server_name *next;
	char *serverName;
	int type;
}_server_name;

typedef struct _index_file {
	struct _index_file *next;
	char *indexFile;
}_index_file;

typedef struct _port {
	struct _port *next;
	int portNum;
	int tls;
}_port;

typedef struct _server {
	struct _server *next;
	_server_name *serverNames;
	_index_file *indexFiles;
	_port *ports;
	_location *locations;
	int tls;
	int autoIndex;
	char *certFile;
	char *keyFile;
	_log_file *accessLog;
	_log_file *errorLog;
}_server;

typedef struct _request {
	char *path;
	char *fullPath;
	char *queryString;
	char *headers;
	char *verb;
	char *protocol;;
	char *host;
	int fd;
	int errorFd;
	int sockFd;
	int isDir;
	SSL *ssl;
	_server *server;
	_location *loc;
}_request;
