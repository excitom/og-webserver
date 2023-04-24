/**
 * Define the datastructures for maintaining server (virtual host)
 * configurations.
 */

#define LOCATION_EXACT 0
#define LOCATION_REGEX 1

typedef struct _ports {
	int portNum;
	int useTLS;
	struct _ports *next;
}_ports;

#define TYPE_PROXY_PASS 0
#define TYPE_DOC_ROOT 1
#define EXACT_MATCH 0
#define REGEX_MATCH 1
#define PREFIX_MATCH 2
typedef struct _location {
	struct _location *next;
	int type;
	int match;
	char *location;
	char *target;		// NULL means inherit from the server
	struct sockaddr_in *passTo;		// for proxy_pass locations
}_location;

typedef struct _server {
	struct _server *next;
	char *serverName;
	char *docRoot;
	short autoIndex;
	int port;
	int tls;
	char *certFile;
	char *keyFile;
	_location *locations;
}_server;

typedef struct _target {
	char *target;
	int type;
}_target;
