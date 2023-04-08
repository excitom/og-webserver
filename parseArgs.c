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
#include "server.h"
#include "global.h"

void
parseArgs(int argc, char* argv[])
{
	g.foreground = 0;
	g.debug = 0;
	g.trace = 0;
	g.useTLS = 0;
	g.dirList = 0;
	g.port = 8080;
	g.epollArraySize = 64;
	int c;
	while ((c = getopt(argc, argv, "dfhlstp:e:")) != EOF)
		switch(c) {
			case 'd':
				g.debug = 1;
				break;
			case 't':
				g.trace = 1;
				break;
			case 'f':
				g.foreground = 1;
				break;
			case 's':
				g.useTLS = 1;
				break;
			case 'l':
				g.dirList = 1;
				break;
			case 'p':
				g.port = atoi(optarg);
				break;
			case 'e':
				g.epollArraySize = atoi(optarg);
				break;
			case 'h':
				printf("Tom's OG web server\n");
				printf("Options are:\n");
				printf("	-f = run in the foreground (else daemon)\n");
				printf("	-d = turn on debugging\n");
				printf("	-t = turn on tracing\n");
				printf("	-h = print this message\n");
				printf("	-l = show directoy listing if missing index file\n");
				printf("	-p = port on which to listen\n");
				printf("	-e = max concurrent epoll events\n");
				printf("	-s = use SSL/TLS\n");
				printf("Positional paramters:\n");
				printf("	config_path - default /etc/ogws\n");
				printf("	doc_root_path - default /www/ogws\n");
				printf("	log_file_path - default /var/log/ogws\n");
				printf("\n");
				exit(0);
			default:
				printf("Unrecognized option %c ignored\n", (char)c);
				break;
		}
	char buffer[BUFF_SIZE];
	//
	// config file path
	//
	if (optind >= argc) {
		strcpy(buffer, "/etc/ogws");
		int len = strlen(buffer);
		g.configPath = malloc(len+1);
		strcpy(g.configPath, buffer);
	} else {
		int len = strlen(argv[optind]);
		g.configPath = malloc(len+1);
		strcpy(g.configPath, argv[optind]);
		optind++;
	}
	if (access(g.configPath, R_OK) == -1) {
		perror("config path not valid:");
		exit(1);
	}

	//
	// doc root file path
	//
	if (optind >= argc) {
		strcpy(buffer, "/www/ogws");
		int len = strlen(buffer);
		g.docRoot = malloc(len+1);
		strcpy(g.docRoot, buffer);
	} else {
		int len = strlen(argv[optind]);
		g.docRoot = malloc(len+1);
		strcpy(g.docRoot, argv[optind]);
		optind++;
	}
	if (access(g.docRoot, R_OK) == -1) {
		perror("doc root not valid:");
		exit(1);
	}

	//
	// log file path
	//
	if (optind >= argc) {
		strcpy(buffer, "/var/log/ogws");
		int len = strlen(buffer);
		g.logPath = malloc(len+1);
		strcpy(g.logPath, buffer);
	} else {
		int len = strlen(argv[optind]);
		g.logPath = malloc(len+1);
		strcpy(g.logPath, argv[optind]);
		optind++;
	}
	if (access(g.logPath, R_OK) == -1) {
		perror("doc root not valid:");
		exit(1);
	}
	snprintf(buffer, BUFF_SIZE, "Listening port: %d\nDocument root: %s \nConfig path: %s\nLog path %s\n", g.port, g.docRoot, g.configPath, g.logPath);
	doDebug(buffer);

	// Temporary - hardcoded cert and key file names
	if (g.useTLS) {
		strcpy(g.certFile, "/halsoft.crt");
		strcpy(g.keyFile, "/halsoft.key");
	} else {
		g.certFile[0] = '\0';
		g.keyFile[0] = '\0';
	}
}
