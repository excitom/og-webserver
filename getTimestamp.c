/**
 * Get a timestamp for the header
 * Option: RESPONSE_FORMAT = response header format
 *         LOG_FILE_FORMAT = log file name format
 *         LOG_RECORD_FORMAT = log file record format
 *
 * (c) Tom Lang 2/2023
 */
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "serverlist.h"
#include "server.h"
#include "global.h"

void
getTimestamp(char *buf, int opt)
{
	time_t rawtime = time(NULL);
	if (rawtime == -1) {
		doDebug("The time() function failed\n");
		return;
	}
	struct tm *ptm = localtime(&rawtime);
	if (ptm == NULL) {
		doDebug("The localtime() function failed\n");
		return;
	}
	memset(buf, 0, TIME_BUF);
	if (opt == RESPONSE_FORMAT) {
		strftime(buf, TIME_BUF, "%a, %d %b %Y %T %Z", ptm);
	}
	else if (opt == LOG_FILE_FORMAT) {
		strftime(buf, TIME_BUF, "%Y%m%d", ptm);
	}
	else if (opt == LOG_RECORD_FORMAT) {
		strftime(buf, TIME_BUF, "%Y%m%d %H:%M:%S ", ptm);
	}
	else {
		doDebug("Unrecognized log record format");
		exit(1);
	}
	return;
}
