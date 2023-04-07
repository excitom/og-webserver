/**
 * SSL Web Server
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
#include <locale.h>
#include "server.h"

// global variables
struct globalVars g;

char* buffer;

/**
 * initialize the SSL server
 */
void
sslServer()
{
	buffer = malloc(BUFF_SIZE);

	SSL_CTX *ctx;
	SSL *ssl = NULL;
	ctx = createContext();
	configureContext(ctx);
	int sockfd = createBindAndListen(g.port);
	while(1) {
		struct sockaddr_in addr;
		socklen_t len = sizeof(addr);
		int clientfd = accept(sockfd, (struct sockaddr*)&addr, &len);
		ssl = SSL_new(ctx);
		SSL_set_fd(ssl, clientfd);
		if ( SSL_accept(ssl) == FAIL ) {     /* do SSL-protocol accept */
			ERR_print_errors_fp(stderr);
			perror("Accept failed");
		} else {
			processInput(clientfd, ssl);
		}
		shutdown(clientfd, SHUT_RDWR);
		close(clientfd);
	}
	close(sockfd);
	SSL_CTX_free(ctx);
	free(buffer);
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
	strcpy(path, g.configPath);
	strcat(path, g.certFile);
	if (SSL_CTX_use_certificate_file(ctx, path, SSL_FILETYPE_PEM) <= 0) {
		fprintf(stderr, "CERT %s\n", path);
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	strcpy(path, g.configPath);
	strcat(path, g.keyFile);
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
