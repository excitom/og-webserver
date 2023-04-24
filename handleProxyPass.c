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
handleProxyPass(char *headers, char *path, char *target)
{
	printf("PROXY PASS %s\nPATH %s\nTARGET %s\n", headers, path, target);
	char *hostname;
	unsigned short port;

}
