/**
 * Turn the web server process into a daemon
 * - set the user name and group name
 * - disassociate from the original process
 * - create new session and process group
 * - move off any mounted file system
 * - create a debug/trace file
 * - redirect stdout and stderr to the debug file
 * - close stdin
 * - save the process id in a file
 *
 * (c) Tom Lang 2/2023
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include "serverlist.h"
#include "server.h"
#include "global.h"

void
daemonize()
{
	// TODO: implement new-style Linux daemon that works with systemd.
	// This implementation is old-style SystemV/BSD.
	if (!g.foreground) {
		// create background process
		pid_t pid = fork();
		if (pid == -1) {
			exit(1);
		}
		else if (pid != 0) {
			exit (0);
		}
		// create new session and process group
		if (setsid ( ) == -1) {
			exit(1);
		}
		// insure disconnected from the tty
		pid = fork();
		if (pid == -1) {
			exit(1);
		}
		else if (pid != 0) {
			exit (0);
		}
		// reset the file creation mask
		umask(0);

		// set the working directory to the root directory 
		// i.e. make sure to move off of a mounted file system
		if (chdir ("/") == -1) {
			exit(1);
		}

		// create a file for debug and trace information
		char path[256];
		char *p = (char *)&path;
		strcpy(p, g.configFile);
		char *q = strrchr(p, '/');
		*q = '\0';
		strcat(p, "/debug.out");
		int fd = open(p, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
		if (fd == -1) {
			int e = errno;
			fprintf(stderr, "%s: file open failed: %s\n", p, strerror(e));
			exit(1);
		}
		//
		// make debug file the default stdout & stderr
		//
		dup2(fd, 2);
		dup2(2,  1);
		close(fd);
	}

	// save the pid in a file
	FILE *fp = fopen(g.pidFile, "w+");
	if (fp == NULL) {
		perror("pid log not valid:");
		exit(1);
	}
	fprintf(fp, "%d\n", getpid() );
	fclose(fp);

	// close stdin
	close(0);
}
