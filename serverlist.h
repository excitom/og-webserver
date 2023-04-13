/**
 * Define the datastructures for maintaining server (virtual host)
 * configurations.
 */

#define LOCATION_EXACT 0
#define LOCATION_REGEX 1

typedef struct _location {
	struct _location *next;
	int match;
		// 0 = exact match
		// 1 = regex match
	char *loction;
	char *root;		// NULL means inherit from the server
}_location;

typedef struct _server {
	struct _server *next;
	char *serverName;
	char *docRoot;
	short autoIndex;
	int port;
	_location *locations;
}_server;
