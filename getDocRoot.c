/**
 * Determine the document root for a URI.
 * - Loop through `location` directives for this server to
 *   find a match.
 * - If no match, use the default doc root for the `server` block.
 * - If no default for the server, use the fallback default.
 *
 * (c) 2023 Tom Lang
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <regex.h>
#include "server.h"
#include "global.h"

int regexMatch(char *, char *);

void
getDocRoot(_server *server, char *path, _location *loc)
{
	loc = server->locations;
	size_t pathLen = strlen(path);
	while (loc != NULL) {
		if ((loc->match == EXACT_MATCH)
				&& (pathLen == strlen(loc->location))
				&& (strcmp(path, loc->location) == 0)) {
			return;
		} else if ((loc->match == REGEX_MATCH)
				&& (regexMatch(loc->location, path))) {
			return;
		} else if ((loc->match == PREFIX_MATCH)
				&& (strncmp(path, loc->location, strlen(loc->location)) == 0)) {
			return;
		}
		loc = loc->next;
	}
	if (server->docRoot == NULL) {
		loc->type = TYPE_DOC_ROOT;
		loc->target = g.defaultServer->docRoot;
	} else {
		loc->type = TYPE_DOC_ROOT;
		loc->target = server->docRoot;
	}
	return;
}

/**
 * regular expression matching
 */
int
regexMatch(char *pattern, char *location)
{
	regex_t regex;
	int ret;
	char msgbuf[100];

	/* Compile regular expression */
	ret = regcomp(&regex, pattern, 0);
	if (ret) {
    	doDebug("Could not compile regex\n");
    	return 0;
	}

	/* Execute regular expression */
	ret = regexec(&regex, location, 0, NULL, 0);
	if (!ret) {
		regfree(&regex);
    	return 1;
	}
	else if (ret == REG_NOMATCH) {
		regfree(&regex);
    	return 0;
	}
	else {
		if (g.debug) {
    		regerror(ret, &regex, msgbuf, sizeof(msgbuf));
    		fprintf(stderr, "Regex match failed: %s\n", msgbuf);
		}
		regfree(&regex);
	}
    return 0;
}