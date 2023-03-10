#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server.h"
#include "global.h"

/**
 * Write to access and error log files
 */
void
accessLog(int sockfd, char *verb, int httpCode, char *path, int size)
{
	unsigned char ts[TIME_BUF];
	getTimestamp((unsigned char *)&ts, LOG_RECORD_FORMAT);

	struct sockaddr_in peeraddr;
	socklen_t len = sizeof(peeraddr);
	getpeername(sockfd, (struct sockaddr*)&peeraddr, &len);
	char peerIp[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &peeraddr.sin_addr.s_addr, peerIp, INET_ADDRSTRLEN);

	char buffer[BUFF_SIZE];
	int sz = snprintf(buffer, BUFF_SIZE, "%s %s %s %d %s %d\n", ts, peerIp, verb, httpCode, path, size);
	write(g.accessFd, buffer, sz);
}

void
errorLog(int sockfd, char *verb, int httpCode, char *path, char *msg)
{
	unsigned char ts[TIME_BUF];
	getTimestamp((unsigned char *)&ts, LOG_RECORD_FORMAT);

	struct sockaddr_in peeraddr;
	socklen_t len = sizeof(peeraddr);
	getpeername(sockfd, (struct sockaddr*)&peeraddr, &len);
	char peerIp[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &peeraddr.sin_addr.s_addr, peerIp, INET_ADDRSTRLEN);

	char buffer[BUFF_SIZE];
	int sz = snprintf(buffer, BUFF_SIZE, "%s %s %s %d %s %s\n", ts, peerIp, verb, httpCode, path, msg);
	write(g.errorFd, buffer, sz);
}

void openLogFiles()
{
	char buffer[BUFF_SIZE];
	char *p = (char *)&buffer;
	unsigned char ts[TIME_BUF];
	getTimestamp((unsigned char *)&ts, LOG_FILE_FORMAT);
	strcpy(p, g.logPath);
	strcat(p, "/access.log.");
	strcat(p, ts);
	g.accessFd = open(p, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
	if (g.accessFd == -1) {
		int e = errno;
		snprintf(buffer, BUFF_SIZE, "%s: file open failed: %s\n", p, strerror(e));
		doDebug(buffer);
		exit(1);
	}
	strcpy(p, g.logPath);
	strcat(p, "/error.log.");
	strcat(p, ts);
	g.errorFd = open(p, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
	if (g.errorFd == -1) {
		int e = errno;
		snprintf(buffer, BUFF_SIZE, "%s: file open failed: %s\n", p, strerror(e));
		doDebug(buffer);
		exit(1);
	}
}

/**
 * Debug print to stderr
 * Note: Assumes a formatted string input.
 */
void
doDebug(unsigned char* buffer) {
	if (!g.debug) {
		return;
	}
	fputs(buffer, stderr);
	if (strchr(buffer, '\n') == NULL) {
		fputc('\n', stderr);
	}
}

/**
 * Verbose trace
 */
void
doTrace (char direction, unsigned char *p, int bytes)
{
	if (!g.trace)
		return;

	int f, l, i;
	unsigned char d[16];
	f = 0;

	while (bytes >= 16) {
		for (i=0; i<16; i++) {
			d[i] = isprint(p[i]) ? p[i] : '.';
		}
		l = f+15;
		fprintf(stderr, "%4.4X - %4.4X %c %2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X - %2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", f, l, direction, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15], d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
		f += 16;
		p += 16;
		bytes -= 16;
	}
	if (bytes > 0) {
		l = f + bytes - 1;
		fprintf(stderr, "%4.4X - %4.4X %c ", f, l, direction);
		for (i = 0; i < bytes; i++) {
			fprintf(stderr, "%2.2X", p[i]);
			d[i] = isprint(p[i]) ? p[i] : '.';
			if (i == 7)
				fprintf(stderr, " - ");
		}
		for (i = 0; i < 32-(bytes*2); i++)
			fprintf(stderr, " ");
		if (bytes<8)
			fprintf(stderr, "   ");
		fprintf(stderr, "  ");
		for (i = 0; i < bytes; i++) {
			fprintf(stderr, "%c", d[i]);
		}
		fputc('\n', stderr);
	}
}
