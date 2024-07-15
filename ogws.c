/**
  * Tom's OG Web Server
  *
  * This is a simple web server which is implemented with an event driven
  * model. There is a single process and a no threads, but everything is
  * done with non-blocking, asynchronous I/O.
  *
  * (c) Tom Lang 2/2023
  */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include<signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <locale.h>
#include "serverlist.h"
#include "server.h"

// global variables
struct globalVars g;

void sendSignal(const char *);
void startProcesses(void);

/**
 * MAIN function
 */
int
main(int argc, char *argv[])
{
	setlocale(LC_NUMERIC, "");
	parseArgs(argc, argv);
	parseConfig();
	if (isShowVersion()) {
		printf("ogws web server version: %s\n", getVersion());
		exit(0);
	}
	// check if a signal should be sent
	const char *sn = getSignalName();
	if (sn != NULL) {
		sendSignal(sn);
		exit(0);
	}
	// exit if only testing the config
	if (isTestConfig()) {
		printf("Config OK\n");
		exit(0);
	}
	parseMimeTypes();
	daemonize();
	startProcesses();
}

/**
 * The service starts one process per port that is listened to, times the
 * number of worker processes from the configuration. This structure keeps
 * track of which port number, whether it is TLS (SSL) or not, and a list
 * of servers that listen on this port for each process.
 *
 * Counterintuitively, it's not necessary to preserve the process ID of the
 * process. When a process is forked it gets a pointer to an element in this
 * list and that is all it needs. 
 *
 * All of the server-related data structures are created as the config file
 * is parsed. Then, processes are started. Due to the semantics of the `fork`
 * system call, each process gets a clone of all the data structures. There
 * is no need for any interprocess communication or synchronization, each
 * process operates independantly of the others.
 *
 * It gets more complicated for a TLS server, where a thread is created to
 * process each request. The threads do not modify any of the server data
 * structures so locking/synchronization is not required.
 */
typedef struct _procs {
	struct _procs *next;
	_server *server;
	int portNum;
	int tls;
}_procs;

static _procs *procList = NULL;

/**
 * While determining how many processes to start we scan the list of servers
 * to see which ports are listened to. This function checks the list of 
 * known ports to see if we already know about this port.
 */
int
uniquePort(int portNum)
{
	_procs *p = procList;
	while (p) {
		if (p->portNum == portNum) {
			return 0;
		}
		p = p->next;
	}
	return 1;
}

/**
 * Start at least 1 process per port on which we are listening.
 * The processes needed per port is controlled by `worker_processes`
 * directive in the config file.
 */
void
startProcesses()
{
	// figure out what ports to assign to the processes
	int pcount = 0;
	for (_server *server = getServerList(); server != NULL; server = server->next) {
		for (_port *port = server->ports; port != NULL; port = port->next) {
			if (uniquePort(port->portNum)) {
				for (int i = 0; i < getWorkerProcesses(); i++) {
					_procs *p = (_procs *)malloc(sizeof(_procs));
					p->portNum = port->portNum;
					p->tls = port->tls;
					p->server = server;
					p->next = procList;
					procList = p;
					pcount++;
				}
			}	
		}
	}

	// We already have 1 process, start pcount-1 more.
	// All the processes will call server() or tlsServer() depending
	// on the `tls` attribute in the port list. Note: we don't support
	// serving both TLS and non-TLS on the same port.
	_procs *p = procList;
	while(--pcount) {
		pid_t pid = fork();
		if (pid <0) {
			perror("Can't fork");
			exit(1);
		}
		// The parent process receives the child pid, child process receives 0.
		// The children break out of the loop and start listening for requests.
		// The parent process forks all the children then falls out of the
		// loop and starts listending for requests.
		if (pid == 0) {
			// Link to the parent process so that killing the parent causes
			// the whole family to die.
			if (prctl(PR_SET_PDEATHSIG, SIGTERM)) {
				perror("Can't link to parent signal");
				exit(1);
			}
			if (isDebug()) {
				fprintf(stderr, "Server Starting, process: %d for port %d\n", pid, p->portNum);
			}
			break;
		}
		p = p->next;
	}
	if (p->tls) {
		tlsServer(p->portNum, p->server);
	} else {
		server(p->portNum, p->server);
	}
	// The servers loop forever, handling requests. We don't expect
	// control to return here, but if it did the process will exit.
}

/**
 * Send a signal to daemon process
 */
#define PID_SIZE 10

void
sendSignal(const char *sn)
{
	const char *pidFile = getPidFile();
	FILE *fp = fopen(pidFile, "r");
	if (fp == NULL) {
		perror("No daemon process found.");
		exit(1);
	}
	char buff[PID_SIZE];
	if (fgets((char *)&buff, PID_SIZE, fp) == NULL) {
		perror("No daemon process found.");
		unlink(pidFile);
		exit(1);
	}
	fclose(fp);
	pid_t pid = atoi(buff);

	if (strcmp(sn, "stop") == 0) {
		kill(pid, SIGKILL);
		unlink(pidFile);
		return;
	}
	else if (strcmp(sn, "quit") == 0) {
		kill(pid, SIGINT);
		unlink(pidFile);
		return;
	}
	else if (strcmp(sn, "reload") == 0) {
		printf("reload not yet implemented\n");
		return;
	}
	else if (strcmp(sn, "reopen") == 0) {
		printf("reopen not yet implemented\n");
		return;
	}
	printf("Unrecognized signal \"%s\"\n", sn);
	return;
}
