/**
 * Tom's OG Web Server
 *
 * This is a simple web server which is implemented with an event driven
 * model. There is a single process and a no threads, but everything is
 * done with non-blocking, asynchronous I/O.
 *
 * (c) Tom Lang 2/2023
 */

char version[] = "0.1.0";

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include<signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <locale.h>
#include "server.h"

// global variables
struct globalVars g;

void sendSignal(void);
void startProcesses(void);

/**
 * MAIN function
 */
int
main(int argc, char *argv[])
{
	setlocale(LC_NUMERIC, "");
	parseArgs(argc, argv, version);
	if (g.showVersion) {
		printf("ogws web server version: %s\n", version);
		exit(0);
	}
	parseConfig(version);
	checkConfig();
	// check if a signal should be sent
	if (g.signal != NULL) {
		sendSignal();
		exit(0);
	}
	// exit if only testing the config
	if (g.testConfig) {
		printf("Config OK\n");
		exit(0);
	}
	parseMimeTypes();
	daemonize();
	startProcesses();
}

/**
 * Start at least 1 process per port on which we are listening.
 * The processes per port is controlled by `worker_processes` directive.
 */
void
startProcesses()
{
	// total processes needed
	int pcount = g.workerProcesses * g.portCount;
	pcount = 1; // debug
	// already have 1, start pcount-1 more.
	// all the processes will call server() or tlsServer()
	while(--pcount) {
		pid_t pid = fork();
		if (pid <0) {
			perror("Can't fork");
			exit(1);
		}
		if (pid == 0) {
			if (g.debug) {
				pid_t pid = getpid();
				fprintf(stderr, "Server Starting, process: %d\n", pid);
			}
			break;
		}
	}
	if (g.useTLS) {
		tlsServer();
	} else {
		server(g.port);
	}
}
/**
 * Send a signal to daemon process
 */
#define PID_SIZE 10

void
sendSignal() {
	FILE *fp = fopen(g.pidFile, "r");
	if (fp == NULL) {
		perror("No daemon process found.");
		exit(1);
	}
	char buff[PID_SIZE];
	if (fgets((char *)&buff, PID_SIZE, fp) == NULL) {
		perror("No daemon process found.");
		exit(1);
	}
	fclose(fp);
	pid_t pid = atoi(buff);

	if (strcmp(g.signal, "stop") == 0) {
		kill(pid, SIGKILL);
		return;
	}
	else if (strcmp(g.signal, "quit") == 0) {
		kill(pid, SIGINT);
		return;
	}
	else if (strcmp(g.signal, "reload") == 0) {
		printf("reload not yet implemented\n");
		return;
	}
	else if (strcmp(g.signal, "reopen") == 0) {
		printf("reopen not yet implemented\n");
		return;
	}
	printf("Unrecognized signal \"%s\"\n", g.signal);
	return;
}
