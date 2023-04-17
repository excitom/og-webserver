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
#include "server.h"
#include "global.h"

// main thread function
void *processRequest( void *);

// thread parameters
typedef struct _request {
	int clientfd;
	SSL_CTX *ctx;
}_request;

// list of active threads
typedef struct _thread {
	struct _thread *next;
	pthread_t thread;
}_thread;

// thread safe counter
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int  threadCount = 0;

/**
 * The SSL server
 */
void
tlsServer()
{
	SSL_CTX *ctx = createContext();
	configureContext(ctx);
	int sockfd = createBindAndListen(g.port);
	_thread *threadList = NULL;
	while(1) {
		struct sockaddr_in addr;
		socklen_t len = sizeof(addr);
		int clientfd = accept(sockfd, (struct sockaddr*)&addr, &len);
		_request request;
		request.clientfd = clientfd;
		request.ctx = ctx;
		pthread_t thread;
		pthread_create(&thread, NULL, processRequest, (void *)&request);
		_thread *t = (_thread*)malloc(sizeof(_thread));
		t->next = threadList;
		threadList = t;
		t->thread = thread;
	}
	close(sockfd);
	SSL_CTX_free(ctx);
	while(threadList != NULL) {
		pthread_join(threadList->thread, NULL);
		threadList = threadList->next;
	}
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
	_request *r = (_request *)param;
	SSL *ssl = SSL_new(r->ctx);
	SSL_set_fd(ssl, r->clientfd);
	if ( SSL_accept(ssl) == FAIL ) {	 /* do SSL-protocol accept */
		ERR_print_errors_fp(stderr);
		perror("Accept failed");
	} else {
		processInput(r->clientfd, ssl);
	}
	shutdown(r->clientfd, SHUT_RDWR);
	close(r->clientfd);
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
 */
void
configureContext(SSL_CTX *ctx)
{
	/* Set the key and cert */
	char *path = malloc(256);
	strcpy(path, g.certFile);
	if (SSL_CTX_use_certificate_file(ctx, path, SSL_FILETYPE_PEM) <= 0) {
		fprintf(stderr, "CERT %s\n", path);
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	strcpy(path, g.keyFile);
	if (SSL_CTX_use_PrivateKey_file(ctx, path, SSL_FILETYPE_PEM) <= 0 ) {
		fprintf(stderr, "KEY %s\n", path);
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