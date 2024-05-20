/**
 * Recursively expand any `include` files in the config file.
 *
 * (c) Tom Lang 8/2023
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "serverlist.h"
#include "server.h"
#include "global.h"

void
expandFile(FILE *in, FILE *out) {
	char str[] = "include";
	char *s = (char *)&str;
	int slen = strlen(s);
	int read;
	size_t len = 0;
	char *line;
	// Copy the config to a temp file, checking each line
	// for an `include` directive.
	while ((read = (int)getline(&line, &len, in)) != -1) {
		char *p = line;
		while((*p == ' ') || (*p == '\t')) {
			p++;
		}
		if (strncmp(p, s, slen) == 0) {
			char *file = p+slen;
			while((*file == ' ') || (*file == '\t')) {
				file++;
			}
			p = strchr(file, ';');
			char *path;
			if (p == NULL) {
				fprintf(stderr, "Invalid 'include' directive, missing semicolon, ignored.\n");
			} else {
				*p = '\0';
				if (file[0] == '/') {
					path = (char *)malloc(strlen(file)+1);
					strcpy(path, file);
				} else {
					path = (char *)malloc(strlen(g.configDir)+strlen(file)+1);
					strcpy(path, g.configDir);
					strcat(path, file);
				}
				FILE *fd = fopen(path, "r");
				if (fd != NULL) {
					if (isDebug()) {
						fprintf(stderr, "Including file %s\n", path);
					}
					expandFile(fd, out);
					fclose(fd);
				} else {
					fprintf(stderr, "Invalid include file path %s, ignored\n", path);
				}
				free(path);
			}
		} else {
			fputs(line, out);
		}
	}
	if (line) {
		free(line);
	}
}

/**
 * Recursively expand include files. Returns the file descriptor
 * of the original conf file if no includes are found, or returns
 * the file descriptor of temp file containing all the expanded
 * content.
 */

FILE *
expandIncludeFiles(char *tempFile) {
	FILE *tempFd = fopen(tempFile, "w");
	if (tempFd == NULL) {
		int e = errno;
		fprintf(stderr, "Cannot create temporary file.\n%s: file open failed: %s\nExiting.\n", tempFile, strerror(e));
		exit(1);
	}
	FILE *fd = fopen(g.configFile, "r");
	if (fd == NULL) {
		int e = errno;
		fprintf(stderr, "Cannot process the configuration.\n%s: file open failed: %s\nExiting.\n", g.configFile, strerror(e));
		exit(1);
	}
	expandFile(fd, tempFd);
	fclose(fd);
	fclose(tempFd);
	return fopen(tempFile, "r");
}
