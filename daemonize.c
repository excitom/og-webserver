#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "server.h"
#include "global.h"

/**
 * Turn the web server process into a daemon
 */
void
daemonize()
{
	//
	// redirect stdout and stderr unless in interactive debug mode
	//
	char buffer[BUFF_SIZE];
	char *p = buffer;
	if (!g.foreground) {
		pid_t pid;

		// create new process
		pid = fork();
		if (pid == -1) {
			exit(1);
		}
		else if (pid != 0) {
			exit (0);
		}

		// save the pid in a file
		strcpy(p, g.configPath);
		strcat(p, "/ogws.pid");
		FILE* fp = fopen(p, "w+");
		fprintf(fp, "%d\n", getpid() );
		fclose(fp);

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
		strcpy(p, g.logPath);
		strcat(p, "/debug.out");
		int fd = open(p, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
		if (fd == -1) {
			int e = errno;
			snprintf(buffer, BUFF_SIZE, "%s: file open failed: %s\n", p, strerror(e));
			doDebug(buffer);
			exit(1);
		}
		//
		// make debug file the default stdout & stderr
		//
		dup2(fd, 2);
		dup2(2,  1);
		close(fd);
	}
	//
	// Open the web server log and error log files
	//
	openLogFiles();

	// close stdin
	close(0);
}
