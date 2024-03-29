/**
 * Parse command line arguments
 *
 * (c) Tom Lang 2/2023
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include "serverlist.h"
#include "server.h"
#include "global.h"

void initGlobals(const char *);

void
parseArgs(int argc, char* argv[], const char *version)
{
	initGlobals(version);
	int c;
	while ((c = getopt(argc, argv, "dfhntvs:")) != EOF)
		switch(c) {
			case 'd':
				g.debug = 1;
				break;
			case 't':
				g.testConfig = 1;
				break;
			case 'f':
				g.foreground = 1;
				break;
			case 'v':
				g.showVersion = 1;
				break;
			case 'n':
				g.noDefaultServer = 1;
				break;
			case 's':
				g.signal = optarg;
				break;
			case 'h':
				printf("Tom's OG web server\n");
				printf("Options are:\n");
				printf("	-f = run in the foreground (else daemon)\n");
				printf("	-d = turn on debugging\n");
				printf("	-s = send a signal to the process, options are:\n");
				printf("	          stop | quit | reload | reopen\n");
				printf("	-t = test the configuration\n");
				printf("	-n = do not launch a default web server\n");
				printf("	-h = print this message\n");
				printf("	-v = show version\n");
				printf("Positional paramter:\n");
				printf("	config_file - default /etc/ogws/ogws.conf\n");
				printf("\n");
				exit(0);
			default:
				printf("Unrecognized option %c ignored\n", (char)c);
				break;
		}
	//
	// config file path
	//
	if (optind >= argc) {
		const char defaultConfigFile[] = "/etc/ogws/ogws.conf";
		int len = strlen(defaultConfigFile);
		g.configFile = malloc(len+1);
		strcpy(g.configFile, defaultConfigFile);
	} else {
		int len = strlen(argv[optind]);
		g.configFile = malloc(len+1);
		strcpy(g.configFile, argv[optind]);
		optind++;
	}
	if (access(g.configFile, R_OK) == -1) {
		perror("config file not valid:");
		exit(1);
	}
	char *p = (char *)malloc(strlen(g.configFile)+1);
	strcpy(p, g.configFile);
	char *q = strrchr(p, '/');
	*++q = '\0';
	g.configDir = p;
}

/**
 * Initialize global variables
 */
void
initGlobals(const char *version)
{
	g.version = (char *)malloc(strlen(version)+1);
	strcpy(g.version, version);
	g.foreground = 0;
	g.debug = 0;
	g.noDefaultServer = 0;
	g.trace = 0;
	g.testConfig = 0;
	g.showVersion = 0;
	g.workerConnections = 64;
	g.workerProcesses = 1;
	g.sendFile = 0;
	g.signal = NULL;
	g.servers = NULL;
	g.portCount = 0;
	const char pidFile[] = "/etc/ogws/ogws.pid";
	g.pidFile = (char *)malloc(strlen(pidFile)+1);
	strcpy(g.pidFile, pidFile);
	const char user[] = "ogws";
	g.user = (char *)malloc(strlen(user)+1);
	strcpy(g.user, user);
	g.group = (char *)malloc(strlen(user)+1);
	strcpy(g.group, user);
	g.accessLogs = NULL;
	g.errorLogs = NULL;
}
