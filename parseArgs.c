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
	// config file and config file path
	//
	char *configFile;
	int len;
	if (optind >= argc) {
		const char defaultConfigFile[] = "/etc/ogws/ogws.conf";
		len = strlen(defaultConfigFile);
		configFile = malloc(len+1);
		strcpy(configFile, defaultConfigFile);
	} else {
		len = strlen(argv[optind]);
		configFile = malloc(len+1);
		strcpy(configFile, argv[optind]);
		optind++;
	}
	if (access(configFile, R_OK) == -1) {
		perror("config file not valid:");
		exit(1);
	}
	setConfigFile(configFile);
	char *p = strdup(configFile);
	char *q = strrchr(p, '/');
	*++q = '\0';
	setConfigDir(p);

	// set defaults
	const char defaultVersion[] = _VERSION;
	len = strlen(defaultVersion);
	char *version = malloc(len+1);
	strcpy(version, defaultVersion);
	setVersion(version);

	const char defaultPidFile[] = "/etc/ogws/ogws.pid";
	len = strlen(defaultPidFile);
	char *pidFile = malloc(len+1);
	strcpy(pidFile, defaultPidFile);
	setPidFile(pidFile);

	const char defaultUser[] = "ogws";
	len = strlen(defaultUser);
	char *user = malloc(len+1);
	strcpy(user, defaultUser);
	setUser(user);

	char *group = strdup(user);
	setGroup(group);
}
