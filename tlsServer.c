/**
 * SSL/TLS Web Server
 *
 * Note: My original design uses `epoll` and asynchronous socket I/O
 * but I was having trouble getting that to work with SSL, so I 
 * switch to a multi-threaded server for SSL. There is one thread
 * launched per incoming request.
 *
 * (c) Tom Lang 2/2023
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <pthread.h>
#include "serverlist.h"
#include "server.h"

int isRetryable(SSL *, int);

// main thread function
void *processRequest( void *);

// thread safe counter
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int  threadCount = 0;

/**
 * The SSL server
 */
void
tlsServer(int portNum, _server *server)
{
	SSL_CTX *ctx = createContext();
	configureContext(ctx, portNum);
	const int isTLS = 1;
	int sockFd = createBindAndListen(isTLS, portNum);
	while(1) {
		struct sockaddr_in addr;
		socklen_t len = sizeof(addr);
		int clientFd;
		while ((clientFd = accept(sockFd, (struct sockaddr *) &addr, &len)) < 0) {
			if ((clientFd < 0) && (errno != EINTR)) {
				fprintf(stderr, "Accept on socket %d failed: %m\n", sockFd);
				cleanup(sockFd);
				return;
			} else {
				fprintf(stderr, "Resuming interrupted `accept()`\n");
			}
		}
		_clientConnection *client = queueClientConnection(clientFd, server, addr, ctx);
		pthread_t thread;
		pthread_create(&thread, NULL, processRequest, (void *)client);
	}
	cleanup(sockFd);
}

/**
 * Thread for handling a request
 */
void *
processRequest(void *param)
{
	printf("Thread started ...\n");
	pthread_mutex_lock(&mutex1);
	threadCount++;
	printf("Thread Count: %d\n", threadCount);
	pthread_mutex_unlock(&mutex1);
	_clientConnection *c = (_clientConnection *)param;
	SSL *ssl = SSL_new(c->ctx);
	SSL_set_fd(ssl, c->fd);
	while (1) {
		int i = SSL_accept(ssl);
		if (i <= 0) {
			if (!isRetryable(ssl, i)) {
				ERR_print_errors_fp(stderr);
				perror("Accept failed");
				break;
			}
		} else {
			_request *req = (_request *)calloc(1,sizeof(_request));
			req->clientFd = c->fd;
			req->server = c->server;
			req->ssl = ssl;
			processInput(req);
			if (req->path) free(req->path);
			if (req->fullPath) free(req->fullPath);
			if (req->queryString) free(req->queryString);
			if (req->headers) free(req->headers);
			if (req->verb) free(req->verb);
			if (req->protocol) free(req->protocol);
			if (req->host) free(req->host);
			free(req);
			break;
		}
	}
	cleanup(c->fd);
	pthread_mutex_lock(&mutex1);
	threadCount--;
	printf("Thread exiting, count: %d\n", threadCount);
	pthread_mutex_unlock(&mutex1);
	return param;
}

/**
 * Create SSL context
 */
SSL_CTX *
createContext()
{
	const SSL_METHOD *method;
	SSL_CTX *ctx;
	method = TLS_server_method();
	OpenSSL_add_all_algorithms();  /* load & register all cryptos, etc. */
	SSL_load_error_strings();   /* load all error messages */

	ctx = SSL_CTX_new(method);
	if (!ctx) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}
	// enable kernel level TLS
	SSL_CTX_set_options(ctx, SSL_OP_ENABLE_KTLS);

	return ctx;
}

/**
 * Configure SSL
 *
 * Note: The server is currently supporting multiple virtual hosts only
 * if they use different ports. SNI (Server Name Indication) support 
 * is TBD.
 */
void
configureContext(SSL_CTX *ctx, int portNum)
{
	_server *server = getServerList();
	while (server) {
		_port *p = server->ports;
		while (p) {
			if (p->portNum == portNum) {
				goto found;		// valid use of a `goto` :-)
			}
			p = p->next;
		}
		server = server->next;
	}
found:
	if (server == NULL) {
		doDebug("Port not found, shouldn't happen");
		exit(EXIT_FAILURE);
	}
	/* Set the key and cert */
	if (SSL_CTX_use_certificate_file(ctx, server->certFile, SSL_FILETYPE_PEM) <= 0) {
		fprintf(stderr, "CERT %s\n", server->certFile);
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, server->keyFile, SSL_FILETYPE_PEM) <= 0 ) {
		fprintf(stderr, "KEY %s\n", server->keyFile);
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	/* verify private key */
	if ( !SSL_CTX_check_private_key(ctx) ) {
		fprintf(stderr, "Private key does not match the public certificate\n");
		exit(EXIT_FAILURE);
	}
}

void showCerts(SSL* ssl) {
	X509 *cert;
	char *line;
	cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */
	if ( cert != NULL )
	{
		printf("Server certificates:\n");
		line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
		printf("Subject: %s\n", line);
		free(line);
		line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
		printf("Issuer: %s\n", line);
		free(line);
		X509_free(cert);
	}
	else
		printf("No certificates.\n");
}

int
isRetryable(SSL *con, int i)
{
    int err = SSL_get_error(con, i);

    /* If it's not a fatal error, it must be retryable */
    return (err != SSL_ERROR_SSL)
           && (err != SSL_ERROR_SYSCALL)
           && (err != SSL_ERROR_ZERO_RETURN);
}
