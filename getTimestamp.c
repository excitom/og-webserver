#include <time.h>
#include <string.h>
#include "server.h"
#include "global.h"

/**
 * Get a timestamp for the header
 */
void
getTimestamp(unsigned char *buf)
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
	strftime(buf, TIME_BUF, "%a, %d %b %Y %T %Z", ptm);
	return;
}
