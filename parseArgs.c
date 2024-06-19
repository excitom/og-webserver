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
#include "version.h"

void
parseArgs(int argc, char* argv[])
{
	int c;
	while ((c = getopt(argc, argv, "dfhntvs:")) != EOF)
		switch(c) {
			case 'd':
				setDebug(true);
				break;
			case 't':
				setTestConfig(true);
				break;
			case 'f':
				setForeground(true);
				break;
			case 'v':
				setShowVersion(true);
				break;
			case 'n':
				setDefaultServer(false);
				break;
			case 's':
				setSignalName(optarg);
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
	char *configFile;
	if (optind >= argc) {
		const char defaultConfigFile[] = "/etc/ogws/ogws.conf";
		int len = strlen(defaultConfigFile);
		configFile = malloc(len+1);
		strcpy(configFile, defaultConfigFile);
	} else {
		int len = strlen(argv[optind]);
		configFile = malloc(len+1);
		strcpy(configFile, argv[optind]);
		optind++;
	}
	if (access(configFile, R_OK) == -1) {
		perror("config file not valid:");
		exit(1);
	}
	char *p = (char *)malloc(strlen(configFile)+1);
	strcpy(p, configFile);
	setConfigFile(p);
	char *q = strrchr(configFile, '/');
	*++q = '\0';
	p = (char *)malloc(strlen(configFile)+1);
	strcpy(p, configFile);
	setConfigDir(p);

	// set defaults
	setVersion(_VERSION);
	setPidFile("/etc/ogws/ogws.pid");
	setUser("ogws");
	setGroup("ogws");
}
