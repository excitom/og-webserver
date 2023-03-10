#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "server.h"
#include "global.h"

/**
 * Get a timestamp for the header
 * Option: 0 = response header format
 *         1 = log file name format
 *         2 = log file record format
 */
void
getTimestamp(unsigned char *buf, int opt)
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
		strftime(buf, TIME_BUF, "%Y%m%d %H:%M:%S", ptm);
	}
	else {
		doDebug("Unrecognized log record format");
		exit(1);
	}
	return;
}
