/**
 * Find an upstream server to process a proxied request
 *
 * (c) Tom Lang 11/2023
 */
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include "serverlist.h"
#include "server.h"

int
getUpstreamServer(_request *req)
{
    struct sockaddr_in server;
	if (req->loc->type & TYPE_UPSTREAM_GROUP) {
		_upstreams *up = req->loc->group;
		if (!up) {
			doDebug("Missing upstream group");
			return -1;
		}
		server.sin_family      = up->currentServer->passTo->sin_family;
		server.sin_port        = up->currentServer->passTo->sin_port;
		server.sin_addr.s_addr = up->currentServer->passTo->sin_addr.s_addr;
		// prepare to handle the next request with the next server
		up->currentServer = up->currentServer->next;
		if (!up->currentServer) {
			up->currentServer = up->servers;	// wrap at end of list
		}

	} else {
		// upstream is a single server
		server.sin_family      = req->loc->passTo->sin_family;
		server.sin_port        = req->loc->passTo->sin_port;
		server.sin_addr.s_addr = req->loc->passTo->sin_addr.s_addr;
	}
	int upstream;
	if ((upstream = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		doDebug("upstream socket failed.");
		doDebug(strerror(errno));
	return -1;
	}
	if (connect(upstream, (struct sockaddr *)&server, sizeof(server)) < 0) {
		doDebug("upstream connect failed.");
		doDebug(strerror(errno));
		close(upstream);
		return -1;
	}
	return upstream;
}
