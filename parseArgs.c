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

void initGlobals();

void
parseArgs(int argc, char* argv[])
{
	initGlobals();
	int c;
	while ((c = getopt(argc, argv, "dfhlstp:")) != EOF)
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
			case 'h':
				printf("Tom's OG web server\n");
				printf("Options are:\n");
				printf("	-f = run in the foreground (else daemon)\n");
				printf("	-d = turn on debugging\n");
				printf("	-t = turn on tracing\n");
				printf("	-h = print this message\n");
				printf("	-l = show directoy listing if missing index file\n");
				printf("	-p = port on which to listen\n");
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
}

/**
 * Initialize global variables
 */
void
initGlobals() {
	g.foreground = 0;
	g.debug = 0;
	g.trace = 0;
	g.useTLS = 0;
	g.useSendfile = 0;
	g.dirList = 0;
	g.port = 8080;
	g.workerConnections = 64;
	g.keepaliveTimeout = 65;
	char configPath[] = "/etc/ogws";
	g.configPath = (char *)malloc(strlen(configPath)+1);
	strcpy(g.configPath, configPath);
	char serverName[] = "_";
	g.serverName = (char *)malloc(strlen(serverName)+1);
	strcpy(g.serverName, serverName);
	char indexFile[] = "index.html";
	g.indexFile = (char *)malloc(strlen(indexFile)+1);
	strcpy(g.indexFile, indexFile);
	char docRoot[] = "/var/www/ogws/html";
	g.docRoot = (char *)malloc(strlen(docRoot)+1);
	strcpy(g.docRoot, docRoot);
	char accessLog[] = "/var/log/ogws/access.log";
	g.accessLog = (char *)malloc(strlen(accessLog)+1);
	strcpy(g.accessLog, accessLog);
	g.serverName = NULL;
	g.certFile = NULL;
	g.keyFile = NULL;
	char pidFile[] = "/run/ogws.pid";
	g.pidFile = (char *)malloc(strlen(pidFile)+1);
	strcpy(g.pidFile, pidFile);
	char user[] = "ogws";
	g.user = (char *)malloc(strlen(user)+1);
	strcpy(g.user, user);
}
