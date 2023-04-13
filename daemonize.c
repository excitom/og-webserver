/**
 * Turn the web server process into a daemon
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
#include "server.h"
#include "global.h"

void
daemonize()
{
	//
	// redirect stdout and stderr unless in interactive debug mode
	//
	if (!g.foreground) {
		if (g.user != NULL) {
			struct passwd *pwd = getpwnam(g.user);;
			if (setgid(pwd->pw_gid) == -1) {
				perror("Can't set GID");
				exit(1);
			}
			if (setuid(pwd->pw_uid) == -1) {
				perror("Can't set UID");
				exit(1);
			}
		}
		pid_t pid;

		// create new process
		pid = fork();
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

		// set the working directory to the root directory 
		// i.e. make sure to move off of a mounted file system
		if (chdir ("/") == -1) {
			exit(1);
		}

		// create a file for debug and trace information
		char path[256];
		char *p = (char *)&path;
		strcpy(p, g.configPath);
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
