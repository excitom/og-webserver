/**
 * Define the datastructures for maintaining server (virtual host)
 * configurations.
 */

#define TYPE_PROXY_PASS 0
#define TYPE_DOC_ROOT 1
#define TYPE_TRY_FILES 2
#define EQUAL_MATCH 0
#define REGEX_MATCH 1
#define PREFIX_MATCH 2
typedef struct _try_target {
	struct _try_target *next;
	char *target;
} _try_target;

typedef struct _location {
	struct _location *next;
	int type;
	char *match;
	char *root;
	int autoIndex;
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

typedef struct _ports {
	struct _ports *next;
	int port;
}_ports;

typedef struct _server {
	struct _server *next;
	_server_name *serverNames;
	_index_file *indexFiles;
	_ports *ports;
	int tls;
	char *listen;
	char *certFile;
	char *keyFile;
	_location *locations;
	char *accessLog;
	char *errorLog;
	int accessFd;
	int errorFd;
}_server;
