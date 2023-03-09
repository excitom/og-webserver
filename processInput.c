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
#include "server.h"
#include "global.h"

/**
 * Process input
 */
void
processInput(int fd)
{
	char *host;
	char *path;
	char *verb;
	char *p;
	char inbuff[BUFF_SIZE];
	unsigned char buffer[BUFF_SIZE];

	int received = recvData(fd, (char *)&inbuff, sizeof(inbuff));
	printf("RECEIVED %d BYTES\n", received);
	printf("%s\n", inbuff);
	if (received <= 0) {
		// no data to read
		doDebug("NO DATA\n");
		shutdown(fd, SHUT_RDWR);
		close(fd);
		return;
	} else {
		verb = (char *)&inbuff;
		p = strchr(verb, ' ');
		*p++ = '\0';
		path = p;
		p = strchr(path, ' ');
		*p++ = '\0';
		host = NULL;
		while(*p++) {
			if (strncmp(p, "Host: ", 6) == 0) {
				break;
			}
		}
		if (p) {
			host = p + 6;
			p = strchr(host, '\r');
			*p = '\0';
		}
		printf("VERB: %s\nPATH: %s\nHOST %s\n", verb, path, host);
		//snprintf(buffer, BUFF_SIZE, "Received %d bytes from fd %d\n", received, fd);
		//doDebug(buffer);

	}
		//unsigned char ts[TIME_BUF];
		//getTimestamp((unsigned char *)&ts);
		//int sz = snprintf(buffer, BUFF_SIZE, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nDate: %s\r\n\r\n<HTML><P>Hello socket %d!</P><P>%s</P>\r\n</HTML>\r\n\r\n", ts, fd, ts);
		//int sent = sendData(fd, buffer, sz);

	return;
}
