/**
 * Linked list of client connections, ordered by socket file descriptor.
 */
#include <stdio.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>

typedef struct _clientConnection {
	int fd;
	char ip[INET_ADDRSTRLEN];
	SSL_CTX *ctx;
	struct _clientConnection *next;
}_clientConnection;
