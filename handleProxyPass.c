/**
 * Reverse-proxy a request to an upstream target.
 *
 * (c) Tom Lang 4/2023
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
#include <netdb.h>
#include "server.h"
#include "global.h"

void
handleProxyPass(int fd, char *headers, char *path, _location *loc)
{
	if (g.debug) {
		fprintf(stderr, "PROXY_PASS to: %s\n", path);
	}
	int upstream = socket(AF_INET, SOCK_STREAM, 0);
	if (upstream < 0) {
		doDebug("upstream socket failed.");
		doDebug(strerror(errno));
		return;
	}
	if (connect(upstream, (struct sockaddr *)loc->passTo, sizeof(loc->passTo)) < 0) {
		doDebug("upstream connect failed.");
		doDebug(strerror(errno));
		return;
	}
	sendData(upstream, NULL, headers, strlen(headers));
	char buffer[BUFF_SIZE];
	size_t bytes = BUFF_SIZE;
	do {
		bytes = recvData(upstream, (char *)&buffer, bytes);
		sendData(fd, NULL, (char *)&buffer, bytes);
		if (bytes == 0) {
			break;
		}
	} while(bytes == BUFF_SIZE);
	return;
}
