%{
// This is the grammar for the config file. It implements a subset of the
// NGINX configuration directives. I use a full-featured sample from
// the NGINX documentation for testing the grammar.
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "parseConfig.h"
#include "serverlist.h"
extern int yylex();
extern FILE *yyin;
void yyerror( const char * );
%}
%define parse.error verbose
%union {
	int iValue;
	char *str;
};
%token <str>  USER;
%token <str>  ERRORLOG;
%token <str>  ACCESSLOG;
%token <str>  LOGFORMAT;
%token <iValue>  WORKERPROCESSES;
%token <str>  INCLUDE;
%token <str>  PID;
%token <str>  PATH;
%token EVENTS;
%token <iValue> WORKERCONNECTIONS;
%token <iValue> WORKERRLIMIT;
%token <str>  QUOTEDSTRING;
%token <str>  HTTP2;
%token <str>  HTTP2L;
%token <str>  HTTPS;
%token <str>  HTTP1;
%token <str>  HTTP;
%token <iValue> KEEPALIVETIMEOUT;
%token <str>  DEFAULTTYPE;
%token <str>  SERVER;
%token <str>  SENDFILE;
%token AUTOINDEX;
%token <str>  TCPNOPUSH;
%token <iValue>  HASHBUCKET;
%token <str>  LISTEN;
%token <str>  SERVERNAME;
%token <str>  DEFAULTSERVER;
%token <str>  LOCATION;
%token <str>  UPSTREAM;
%token <str>  ROOT;
%token <str>  PROXYPASS;
%token <str>  FASTCGIPASS;
%token <str>  RETURN;
%token WEIGHT;
%token <str>  EXPIRES;
%token <str>  REWRITE;
%token <str>  ERRORPAGE;
%token <str>  LOGNOTFOUND;
%token <str>  TRYFILES;
%token <str>  SSLCERTIFICATEKEY;
%token <str>  SSLCERTIFICATE;
%token SSL_;
%token TRACE;
%token REUSEPORT;
%token INDEX;
%token ON;
%token OFF;
%token <str> MAIN;
%token EOL;
%token <str>  IP;
%token <str>  UNITS;
%token <iValue> PORT;
%token <iValue> NUMBER;
%token <str>  NAME;
%token <str>  VARIABLE;
%token <str>  PREFIXNAME;
%token <str>  SUFFIXNAME;
%token EQUAL_OPERATOR;
%token <str>  REGEXP;
%token WILDCARD;
%%
config
	: main_directives events_section http_section
	{f_config_complete();}
	| events_section http_section
	{f_config_complete();}
	;
main_directives
	: main_directive main_directives
	| main_directive
	;
main_directive
	: user_directive
	| access_log_directive
	| error_log_directive
	| pid_directive
	| include_directive
	| trace_directive
	| worker_processes_directive
	| worker_rlimit_nofile_directive
	;
user_directive
	:
	USER NAME NAME EOL
	{f_user($2, $3);}
	|
	USER NAME EOL
	{f_user($2, NULL);}
	;
pid_directive
	:
	PID PATH EOL
	{f_pid($2);}
	;
include_directive
	:
	INCLUDE PATH EOL
	{printf("UNIMPLEMENTED Include path: %s\n", $2);}
	;
trace_directive
	: TRACE ON EOL
	{f_trace(1);}
	|
	TRACE OFF EOL
	{f_trace(0);}
	;
worker_processes_directive
	: WORKERPROCESSES NUMBER EOL
	{f_workerProcesses($2);}
	;
worker_rlimit_nofile_directive
	: WORKERRLIMIT NUMBER EOL
	{printf("UNIMPLEMENTED Worker rlimit number of files: %d\n", $2);}
	;
events_section
	: EVENTS '{' events_directives '}'
	| EVENTS '{' '}'
	;
events_directives
	: events_directives events_directive
	| events_directive
	;
events_directive
	: WORKERCONNECTIONS NUMBER EOL
	{f_workerConnections($2);}
	;
http_section
	: HTTP '{' http_directives '}'
	{f_http();}
	;
http_directives
	: http_directives http_directive
	| http_directive
	;
http_directive
	: include_directive
	| index_directive
	| default_type_directive
	| access_log_directive
	| error_log_directive
	| log_format_directive
	| sendfile_directive
	| tcp_nopush_directive
	| keepalive_directive
	| server_names_hash_bucket_size_directive
	| server_section
	| upstream_directive
	;
index_directive
	: INDEX index_files index_file EOL
	| INDEX index_file EOL
	;
index_files
	: index_files index_file
	| index_file
	;
index_file
	:
	NAME
	{f_indexFile($1);}
	;
default_type_directive
	:
	DEFAULTTYPE PATH EOL
	{printf("NOT YET IMPLEMENTED: Default type: %s\n", $2);}
	;
sendfile_directive
	:
	SENDFILE ON EOL
	{f_sendfile(1);}
	|
	SENDFILE OFF EOL
	{f_sendfile(0);}
	;
tcp_nopush_directive
	:
	TCPNOPUSH ON EOL
	{f_tcpnopush(1);}
	|
	TCPNOPUSH OFF EOL
	{f_tcpnopush(0);}
	;
keepalive_directive
	:
	KEEPALIVETIMEOUT NUMBER EOL
	{f_keepalive_timeout($2);}
	;
server_names_hash_bucket_size_directive
	:
	HASHBUCKET NUMBER EOL
	{printf("NOT YET IMPLEMENTED Server names hash bucket size: %d\n", $2);}
	;
log_format_directive
	:
	LOGFORMAT MAIN strings EOL
	{printf("NOT YET IMPLEMENTED MAIN log format\n");}
	|
	LOGFORMAT strings EOL
	;
strings
	: strings QUOTEDSTRING
	{printf("NOT YET IMPLEMENTED log format part %s\n", $2);}
	| QUOTEDSTRING
	{printf("NOT YET IMPLEMENTED log format part %s\n", $1);}
	;
server_section
	:
	SERVER '{' server_directives '}'
	{f_server();}
	|
	SERVER '{' '}'
	{f_server();}
	;
server_directives
	: server_directives server_directive
	| server_directive
	;
server_directive
	: server_name_directive
	| access_log_directive
	| error_log_directive
	| root_directive
	| autoindex_directive
	| location_section
	| listen_directive
	| ssl_directive
	| index_directive
	;
server_name_directive
	: SERVERNAME server_names EOL
	;
server_names
	: server_names server_name
	| server_name
	;
server_name
	:
	PREFIXNAME
	{f_server_name($1, SERVER_NAME_WILDCARD_PREFIX);}
	|
	SUFFIXNAME
	{f_server_name($1, SERVER_NAME_WILDCARD_SUFFIX);}
	|
	NAME
	{f_server_name($1, SERVER_NAME_EXACT);}
	;
upstream_directive
	: UPSTREAM NAME '{' upstreams '}'
	{printf("UNIMPLEMENTED Upstream to %s\n", $2);}
	;
upstreams
	:
	upstreams upstream
	|
	upstream
	;
upstream
	:
	SERVER IP PORT WEIGHT EQUAL_OPERATOR NUMBER EOL
	{printf("UNIMPLEMENTED Upstream IP %s port %d weight %d\n", $2, $3, $6);}
	|
	SERVER IP PORT EOL
	{printf("UNIMPLEMENTED Upstream IP %s port %d\n", $2, $3);}
	;
access_log_directive
	:
	ACCESSLOG PATH EOL
	{f_access_log($2, 0);}
	| ACCESSLOG PATH MAIN EOL
	{f_access_log($2, 1);}
	;
error_log_directive
	:
	ERRORLOG PATH EOL
	{f_error_log($2);}
	;
root_directive
	:
	ROOT PATH EOL
	{f_root($2);}
	|
	ROOT NAME EOL
	{f_root($2);}
	;
ssl_directive
	:
	SSLCERTIFICATE PATH EOL
	{f_ssl_certificate($2);}
	|
	SSLCERTIFICATEKEY PATH EOL
	{f_ssl_certificate_key($2);}
	;
autoindex_directive
	:
	AUTOINDEX ON EOL
	{f_autoindex(1);}
	|
	AUTOINDEX OFF EOL
	{f_autoindex(0);}
	;
location_section
	:
	LOCATION EQUAL_OPERATOR PATH '{' location_directives '}'
	{f_location(EQUAL_MATCH, $3);}
	|
	LOCATION REGEXP '{' location_directives '}'
	{f_location(REGEX_MATCH, $2);}
	|
	LOCATION PATH '{' location_directives '}'
	{f_location(PREFIX_MATCH, $2);}
	;
location_directives
	: location_directives location_directive
	| location_directive
	;
location_directive
	: root_directive
	| proxy_pass_directive
	| fastcgi_pass
	| expires_directive
	| try_files_directive
	;
proxy_pass_directive
	: PROXYPASS protocol NAME PORT EOL
	{f_proxy_pass($3, $4);}
	| PROXYPASS protocol IP PORT EOL
	{f_proxy_pass($3, $4);}
	| PROXYPASS protocol NAME EOL
	{f_proxy_pass($3, 0);}
	| PROXYPASS protocol IP EOL
	{f_proxy_pass($3, 0);}
	;
fastcgi_pass
	: FASTCGIPASS NAME PORT EOL
	{printf("UNIMPLEMENTED fastCGI pass to %s on port %d\n", $2, $3);}
	| FASTCGIPASS IP PORT EOL
	{printf("UNIMPLEMENTED fastCGI pass to %s on port %d\n", $2, $3);}
	| FASTCGIPASS NAME EOL
	{printf("UNIMPLEMENTED fastCGI pass to %s\n", $2);}
	| FASTCGIPASS IP EOL
	{printf("UNIMPLEMENTED fastCGI pass to IP %s\n", $2);}
	;
try_files_directive
	:
	TRYFILES try_paths EOL
	{f_try_files();}
	;
try_paths
	:
	try_paths PATH
	{f_try_target($2);}
	|
	PATH
	{f_try_target($1);}
	|
	try_paths NAME
	{f_try_target($2);}
	|
	NAME
	{f_try_target($1);}
	|
	try_paths VARIABLE
	{f_try_target($2);}
	|
	VARIABLE
	{f_try_target($1);}
	;
protocol
	: HTTP1
	{f_protocol("http");}
	| HTTPS
	{f_protocol("https");}
	| HTTP2
	{f_protocol("http2");}
	;
expires_directive
	: EXPIRES UNITS EOL
	{f_expires($2);}
	| EXPIRES OFF EOL
	{f_expires("off");}
	;
listen_directive
	: LISTEN listen_options EOL
	;
listen_options
	: listen_options listen_option
	| listen_option
	;
listen_option
	: NUMBER
	{f_listen(NULL, $1);}
	| NAME
	{f_listen($1, 0);}
	| WILDCARD
	{f_listen("*", 0);}
	| DEFAULTSERVER
	{printf("UNIMPLEMENTED This is the default server\n");}
	| SSL_
	{f_tls();}
	| HTTP2L
	{printf("UNIMPLEMENTED This server uses HTTP2\n");}
	| REUSEPORT
	{printf("UNIMPLEMENTED Reuse port for this server\n");}
	| listen_pair
	;
listen_pair
	: IP PORT
	{f_listen($1, $2);}
	| NAME PORT
	{f_listen($1, $2);}
	| WILDCARD PORT
	{f_listen("*", $2);}
	;
%%
#ifdef standalone
int main( int argc, char **argv )
{
  yyparse();
  return 0;
}
// config parser stub functions for testing in stand alone mode
void f_pid(char *path) {
	printf("PID file %s\n", path);
}
void f_trace(int flag) {
	if (flag) {
		printf("Trace ON\n");
	} else {
		printf("Trace OFF\n");
	}
}
void f_autoindex(int flag) {
	if (flag) {
		printf("Auto index ON\n");
	} else {
		printf("Auto index OFF\n");
	}
}
void f_sendfile(int flag) {
	if (flag) {
		printf("Sendfle ON\n");
	} else {
		printf("Sendfle OFF\n");
	}
}
void f_tcpnopush(int flag) {
	if (flag) {
		printf("TCP no push ON\n");
	} else {
		printf("TCP no push OFF\n");
	}
}
void f_user(char *user, char *group) {
	printf("User name %s\n", user);
	if (group) {
		printf("Group name %s\n", group);
	}
}
void f_server() {
	printf("SERVER section\n");
}
void f_http() {
	printf("HTTP section\n");
}
void f_root(char *path) {
	printf("Document root: %s\n", path);
}
void f_expires(char *expires) {
	printf("Expires: %s\n", expires);
}
void f_server_name(char *name, int type) {
	printf("Server name: %s type %d\n", name, type);
}
void f_indexFile(char *indexFile) {
	printf("Index file directive %s\n", indexFile);
}
void f_error_log(char *path) {
	printf("Error log path %s\n", path);
}
void f_access_log(char *path, int type) {
	printf("Access log path %s type %d\n", path, type);
}
void f_ssl_certificate(char *cert) {
	printf("SSL cert %s\n", cert);
}
void f_ssl_certificate_key(char *key) {
	printf("SSL key %s\n", key);
}
void f_listen(char *n, int p) {
	if (p > 0) {
		printf("Listen on port %d\n", p);
	}
	if (n != NULL) {
		printf("Listen to %s\n", n);
	}
}
void f_tls() {
	printf("Use TLS/SSL\n");
}

void f_location(int type, char *match) {
	if (type == EQUAL_MATCH) {
		printf("Location Exact match %s\n", match);
	} else if (type == REGEX_MATCH) {
		printf("Location regex match %s\n", match);
	} else if (type == PREFIX_MATCH) {
		printf("Location prefix match %s\n", match);
	} else {
		printf("Location : unknown match type\n");
	}
}
void f_try_target(char *target) {
	printf("Try files target %s\n", target);
}
void f_try_files() {
	printf("Try files\n");
}
void f_proxy_pass(char *host, int port) {
	if (port) {
		printf("Proxy pass to %s:%d\n", host, port);
	} else {
		printf("Proxy pass to %s\n", host);
	}	
}
void f_protocol(char *protocol) {
	printf("Proxy pass tp %s protocol\n", protocol);
}
void f_keepalive_timeout(int timeout) {
	printf("Keepalive timeout %d\n", timeout);
}
void f_workerProcesses(int num) {
	printf("Worker proceses %d\n", num);
}
void f_workerConnections(int num) {
	printf("Worker connections %d\n", num);
}
void f_events() {
	printf("Events section\n");
}
void f_config_complete() {
	printf("Config file processing complete\n");
}
#endif
