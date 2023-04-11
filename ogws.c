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
#include <errno.h>
#include <sys/types.h>
#include <locale.h>
#include "server.h"

// global variables
struct globalVars g;

/**
 * MAIN function
 */
int
main(int argc, char *argv[])
{
	setlocale(LC_NUMERIC, "");
	parseArgs(argc, argv);
	if (g.showVersion) {
		printf("ogws web server version: %s\n", version);
		exit(0);
	}
	parseConfig();
	checkConfig();
	// exit if only testing the config
	if (g.testConfig) {
		printf("Config OK\n");
		exit(0);
	}
	daemonize();
	parseMimeTypes();

	if (g.useTLS) {
		sslServer();
	} else {
		server();
	}
}
