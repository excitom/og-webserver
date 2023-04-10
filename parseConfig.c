/**
 * Parse the config file.
 *
 * The file format matches the nginx config file format.
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
#include "server.h"
#include "global.h"

void removeComments(char *);

void
parseConfig() {
	char *fileName = malloc(256);
	strcpy(fileName, g.configPath);
	strcat(fileName, "/ogws.conf");
	int fd = open(fileName, O_RDONLY);
	if (fd == -1) {
		int e = errno;
		char buffer[BUFF_SIZE];
		snprintf(buffer, BUFF_SIZE, "%s: file open failed: %s\n", fileName, strerror(e));
		doDebug(buffer);
		exit(1);
	}

	// allocate space to hold the whole file
  	int size = lseek(fd, 0, SEEK_END);
	char *data = malloc(size+1);
	char *p = data;
	if (p == NULL) {
		perror("Out of memory");
		exit(1);
	}
	memset(p, 0, size+1);
	lseek(fd, 0, SEEK_SET);
	int sz = read(fd, p, size);
	if (size != sz) {
		fprintf(stderr, "Can't read config file: %d != %d\n", size, sz);
		exit(1);
	}
	if (g.debug) {
		fprintf(stderr, "Config file size: %d\n", size);
	}
	removeComments(p);

	// free up the space for the file and file name
	free(data);
	free(fileName);
}

/**
 * Remove comments lines so that they are ignored. Comments are
 * removed by changing them to blank space.
 * A comment starts with a '#' and ends with a newline character.
 */
void
removeComments(char *p)
{
	while(p) {
		if (*p == '#') {
			while(*p != '\n') {
				*p++ = ' ';
			}
		} else {
			p++;
		}
	}
}
