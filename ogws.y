%{
#include <stdio.h>
#include <string.h>
#include <ctype.h>
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
%token <iValue>  WORKERPROCESSES;
%token <str>  INCLUDE;
%token <str>  PID;
%token <str>  PATH;
%token EVENTS;
%token <iValue> WORKERCONNECTIONS;
%token <str>  HTTP2;
%token <str>  HTTPS;
%token <str>  HTTP1;
%token <str>  HTTP;
%token <iValue> KEEPALIVETIMEOUT;
%token <str>  DEFAULTTYPE;
%token <str>  SERVER;
%token <str>  SENDFILE;
%token AUTOINDEX;
%token <str>  TCPNOPUSH;
%token <str>  LISTEN;
%token <str>  SERVERNAME;
%token <str>  DEFAULTSERVER;
%token <str>  LOCATION;
%token <str>  ROOT;
%token <str>  PROXYPASS;
%token <str>  RETURN;
%token <str>  EXPIRES;
%token <str>  REWRITE;
%token <str>  ERRORPAGE;
%token <str>  LOGNOTFOUND;
%token <str>  TRYFILES;
%token <str>  SSLCERTIFICATEKEY;
%token <str>  SSLCERTIFICATE;
%token SSL;
%token TRACE;
%token <str>  REUSEPORT;
%token <str>  INDEX;
%token <str>  ON;
%token <str>  OFF;
%token <str>  MAIN;
%token TILDE;
%token EOL;
%token COLON;
%token EQUAL;
%token SLASH;
%token STAR;
%token <str>  IP;
%token <str>  UNITS;
%token <iValue> NUMBER;
%token <str>  NAME;
%token <str>  PREFIXNAME;
%token <str>  SUFFIXNAME;
%%
config:
	  main_directives events_section http_section
	{printf("Complete\n");}
	;
main_directives: main_directive main_directives
	| main_directive
	;
main_directive: user_directive
	| access_log_directive
	| error_log_directive
	| pid_directive
	| include_directive
	| trace_directive
	| worker_processes_directive
	;
user_directive:
	USER NAME EOL
	{printf("User Name %s\n", $2);}
	;
pid_directive:
	PID PATH EOL
	{printf("PID file path %s\n", $2);}
	;
include_directive:
	INCLUDE PATH EOL
	{printf("Include path: %s\n", $2);}
	;
trace_directive:
	TRACE ON EOL
	{printf("Trace ON\n");}
	|
	TRACE OFF EOL
	{printf("Trace OFF\n");}
	;
worker_processes_directive:
	WORKERPROCESSES NUMBER EOL
	{printf("Worker processes: %d\n", $2);}
	;
events_section: EVENTS '{' events_directives '}';
events_directives: events_directives events_directive
	| events_directive
	;
events_directive:
	WORKERCONNECTIONS NUMBER EOL
	{printf("Worker connections %d\n", $2);}
	;
http_section: HTTP '{' http_directives servers '}';
http_directives: http_directives http_directive
	| http_directive
	;
http_directive: include_directive
	| index_directive
	| default_type_directive
	| access_log_directive
	| error_log_directive
	| sendfile_directive
	| tcp_nopush_directive
	| keepalive_directive
	;
index_directive: INDEX index_files index_file EOL
	| INDEX index_file EOL
	;
index_files: index_files index_file
		| index_file
		;
index_file:
	PATH
	{printf("Index file name: %s\n", $1);}
	;
default_type_directive:
	DEFAULTTYPE PATH EOL
	{printf("Default type: %s\n", $2);}
	;
sendfile_directive:
	SENDFILE ON EOL
	{printf("Use sendfile ON\n");}
	|
	SENDFILE OFF EOL
	{printf("Use sendfile OFF\n");}
	;
tcp_nopush_directive:
	TCPNOPUSH ON EOL
	{printf("TCP nopush ON\n");}
	|
	TCPNOPUSH OFF EOL
	{printf("TCP nopush OFF\n");}
	;
keepalive_directive:
	KEEPALIVETIMEOUT NUMBER EOL
	{printf("Keepalive Timeout: %d\n", $2);}
	;
servers: servers server_section
	| server_section
	;
server_section: SERVER '{' server_directives locations '}';
server_directives: server_directives server_directive
	| server_directive
	;
server_directive: listen_directive
	| server_name_directive
	| access_log_directive
	| error_log_directive
	| root_directive
	| autoindex_directive
	;
listen_directive: LISTEN IP COLON NUMBER EOL
	| LISTEN IP COLON NUMBER listen_options EOL
	| LISTEN IP EOL
	| LISTEN IP listen_options EOL
	| LISTEN NUMBER EOL
	| LISTEN NUMBER listen_options EOL
	;
listen_options: listen_options listen_option
	| listen_option
	;
listen_option: DEFAULTSERVER
	| SSL
	| HTTP2
	| REUSEPORT
	;
server_name_directive: SERVERNAME server_names EOL;
server_names: server_names server_name
	| server_name
	;
server_name:
	PREFIXNAME
	{printf("Wildcard PREFIX Server name: %s\n", $1);}
	|
	SUFFIXNAME
	{printf("Wildcard SUFFIX Server name: %s\n", $1);}
	|
	NAME
	{printf("Server name: %s\n", $1);}
	;
access_log_directive:
	ACCESSLOG PATH EOL
	{printf("Access log path: %s\n", $2);}
	| ACCESSLOG PATH MAIN EOL
	{printf("Access log path: %s MAIN %s\n", $2, $3);}
	;
error_log_directive:
	ERRORLOG PATH EOL
	{printf("Error log path: %s\n", $2);}
	;
root_directive:
	ROOT PATH EOL
	{printf("Document root: %s\n", $2);}
	;
autoindex_directive:
	AUTOINDEX ON EOL
	{printf("Auto indexing ON\n");}
	|
	AUTOINDEX OFF EOL
	{printf("Auto indexing OFF\n");}
	;
locations: locations location
	| location
	;
location: LOCATION '{' location_directives '}';
location_directives: location_directives location_directive
	| location_directive
	;
location_directive: root_directive
	| proxy_pass_directive
	| expires_directive
	;
proxy_pass_directive: PROXYPASS protocol COLON SLASH SLASH PATH EOL
	| PROXYPASS protocol COLON SLASH SLASH IP EOL
	;
protocol: HTTP1
	| HTTPS
	| HTTP2
	;
expires_directive: EXPIRES UNITS EOL;
%%
extern FILE *yyin;
int main( int argc, char **argv )
{
  yyparse();
  return 0;
}
