%{
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
extern int yylex();
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
%token SSL;
%token TRACE;
%token REUSEPORT;
%token INDEX;
%token ON;
%token OFF;
%token <str> MAIN;
%token <str> LOC_OPERATOR;
%token EOL;
%token <str>  IP;
%token <str>  UNITS;
%token <iValue> PORT;
%token <iValue> NUMBER;
%token <str>  NAME;
%token <str>  PREFIXNAME;
%token <str>  SUFFIXNAME;
%token <str>  REGEXP;
%token WILDCARD;
%%
config
	: main_directives events_section http_section
	{printf("Complete\n");}
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
	{printf("User Name %s group %s\n", $2, $3);}
	|
	USER NAME EOL
	{printf("User Name %s\n", $2);}
	;
pid_directive
	:
	PID PATH EOL
	{printf("PID file path %s\n", $2);}
	;
include_directive
	:
	INCLUDE PATH EOL
	{printf("Include path: %s\n", $2);}
	;
trace_directive
	: TRACE ON EOL
	{printf("Trace ON\n");}
	|
	TRACE OFF EOL
	{printf("Trace OFF\n");}
	;
worker_processes_directive
	: WORKERPROCESSES NUMBER EOL
	{printf("Worker processes: %d\n", $2);}
	;
worker_rlimit_nofile_directive
	: WORKERRLIMIT NUMBER EOL
	{printf("Worker rlimit number of files: %d\n", $2);}
	;
events_section
	: EVENTS '{' events_directives '}'
	;
events_directives
	: events_directives events_directive
	| events_directive
	;
events_directive
	: WORKERCONNECTIONS NUMBER EOL
	{printf("Worker connections %d\n", $2);}
	;
http_section
	: HTTP '{' http_directives '}'
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
	{printf("Index file name: %s\n", $1);}
	;
default_type_directive
	:
	DEFAULTTYPE PATH EOL
	{printf("Default type: %s\n", $2);}
	;
sendfile_directive
	:
	SENDFILE ON EOL
	{printf("Use sendfile ON\n");}
	|
	SENDFILE OFF EOL
	{printf("Use sendfile OFF\n");}
	;
tcp_nopush_directive
	:
	TCPNOPUSH ON EOL
	{printf("TCP nopush ON\n");}
	|
	TCPNOPUSH OFF EOL
	{printf("TCP nopush OFF\n");}
	;
keepalive_directive
	:
	KEEPALIVETIMEOUT NUMBER EOL
	{printf("Keepalive Timeout: %d\n", $2);}
	;
server_names_hash_bucket_size_directive
	:
	HASHBUCKET NUMBER EOL
	{printf("Server names hash bucket size: %d\n", $2);}
	;
log_format_directive
	:
	LOGFORMAT MAIN strings EOL
	{printf("MAIN log format\n");}
	|
	LOGFORMAT strings EOL
	;
strings
	: strings QUOTEDSTRING
	{printf("log format part %s\n", $2);}
	| QUOTEDSTRING
	{printf("log format part %s\n", $1);}
	;
server_section
	:
	SERVER '{' server_directives '}'
	{printf("Server SECTION\n");}
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
	{printf("Wildcard PREFIX Server name: %s\n", $1);}
	|
	SUFFIXNAME
	{printf("Wildcard SUFFIX Server name: %s\n", $1);}
	|
	NAME
	{printf("Server name: %s\n", $1);}
	;
upstream_directive
	: UPSTREAM NAME '{' upstreams '}'
	{printf("Upstream to %s\n", $2);}
	;
upstreams
	:
	upstreams upstream
	|
	upstream
	;
upstream
	:
	SERVER IP PORT WEIGHT LOC_OPERATOR NUMBER EOL
	{printf("Upstream IP %s port %d weight %d\n", $2, $3, $6);}
	|
	SERVER IP PORT EOL
	{printf("Upstream IP %s port %d\n", $2, $3);}
	;
access_log_directive
	:
	ACCESSLOG PATH EOL
	{printf("Access log path: %s\n", $2);}
	| ACCESSLOG PATH MAIN EOL
	{printf("Access log path: %s MAIN %s\n", $2, $3);}
	;
error_log_directive
	:
	ERRORLOG PATH EOL
	{printf("Error log path: %s\n", $2);}
	;
root_directive
	:
	ROOT PATH EOL
	{printf("Document root: %s\n", $2);}
	|
	ROOT NAME EOL
	{printf("Document root: %s\n", $2);}
	;
autoindex_directive
	:
	AUTOINDEX ON EOL
	{printf("Auto indexing ON\n");}
	|
	AUTOINDEX OFF EOL
	{printf("Auto indexing OFF\n");}
	;
location_section
	:
	LOCATION LOC_OPERATOR PATH '{' location_directives '}'
	{printf("LOCATION operator %s path %s\n", $2, $3);}
	|
	LOCATION REGEXP '{' location_directives '}'
	{printf("LOCATION REGEXP %s\n", $2);}
	|
	LOCATION PATH '{' location_directives '}'
	{printf("LOCATION implicit prefix match %s\n", $2);}
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
	;
proxy_pass_directive
	: PROXYPASS protocol NAME PORT EOL
	{printf("proxy pass to %s on port %d\n", $3, $4);}
	| PROXYPASS protocol IP PORT EOL
	{printf("proxy pass to %s on port %d\n", $3, $4);}
	| PROXYPASS protocol NAME EOL
	{printf("proxy pass to %s\n", $3);}
	| PROXYPASS protocol IP EOL
	{printf("proxy pass to IP %s\n", $3);}
	;
fastcgi_pass
	: FASTCGIPASS NAME PORT EOL
	{printf("fastCGI pass to %s on port %d\n", $2, $3);}
	| FASTCGIPASS IP PORT EOL
	{printf("fastCGI pass to %s on port %d\n", $2, $3);}
	| FASTCGIPASS NAME EOL
	{printf("fastCGI pass to %s\n", $2);}
	| FASTCGIPASS IP EOL
	{printf("fastCGI pass to IP %s\n", $2);}
	;
protocol
	: HTTP1
	| HTTPS
	| HTTP2
	;
expires_directive
	: EXPIRES UNITS EOL
	{printf("Expires %s\n", $2);}
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
	{printf("Listen on PORT %d\n", $1);}
	| NAME
	{printf("Listen to %s\n", $1);}
	| WILDCARD
	{printf("Listen WILDCARD\n");}
	| PREFIXNAME
	{printf("Listen to wildcard %s\n", $1);}
	| SUFFIXNAME
	{printf("Listen to wildcard suffix %s\n", $1);}
	| DEFAULTSERVER
	{printf("This is the default server\n");}
	| SSL
	{printf("This server uses SSL/TLS\n");}
	| HTTP2L
	{printf("This server uses HTTP2\n");}
	| REUSEPORT
	{printf("Reuse port for this server\n");}
	| listen_pair
	;
listen_pair
	: IP PORT
	{printf("Listen at %s on port %d\n", $1, $2);}
	| NAME PORT
	{printf("Listen to %s on port %d\n", $1, $2);}
	| PREFIXNAME PORT
	{printf("Listen to wildcard %s on port %d\n", $1, $2);}
	| SUFFIXNAME PORT
	{printf("Listen to wildcard suffix %s on port %d\n", $1, $2);}
	| WILDCARD PORT
	{printf("Listen to WILDCARD on port %d\n", $2);}
	;
%%
extern FILE *yyin;
#ifdef standalone
int main( int argc, char **argv )
{
  yyparse();
  return 0;
}
#endif
