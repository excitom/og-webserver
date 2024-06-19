%{
// This is the lexical analyzer for parsing the config file.
// It implements a subset of the NGINX configuration directives.
// I use a full-featured sample from the NGINX documentation
// for testing the grammar.
#include <stdio.h>
#include "og_ws.tab.h"
extern int main(int, char **);
extern YYSTYPE yylval;
%}
%option yylineno
%%
server_names_hash_bucket_size {yylval.iValue = atoi(yytext); return HASHBUCKET;}
fastcgi_split_path_info	{yylval.str = strdup(yytext); return FASTCGISPLITPATHINFO;}
worker_rlimit_nofile {yylval.iValue = atoi(yytext); return WORKERRLIMIT;}
ssl_certificate_key	{yylval.str = strdup(yytext); return SSLCERTIFICATEKEY;}
ssl_session_timeout	{yylval.str = strdup(yytext); return SSLSESSIONTIMEOUT;}
ssl_session_tickets	{yylval.str = strdup(yytext); return SSLSESSIONTICKETS;}
worker_connections	{yylval.iValue = atoi(yytext); return WORKERCONNECTIONS;}
keepalive_timeout	{yylval.iValue = atoi(yytext); return KEEPALIVETIMEOUT;}
ssl_session_cache	{yylval.str = strdup(yytext); return SSLSESSIONCACHE;}
worker_processes	{yylval.iValue = atoi(yytext); return WORKERPROCESSES;}
ssl_certificate	{yylval.str = strdup(yytext); return SSLCERTIFICATE;}
default_server	{yylval.str = strdup(yytext); return DEFAULTSERVER;}
fastcgi_index	{yylval.str = strdup(yytext); return FASTCGIINDEX;}
fastcgi_param	{yylval.str = strdup(yytext); return FASTCGIPARAM;}
log_not_found	{yylval.str = strdup(yytext); return LOGNOTFOUND;}
server_tokens	{yylval.str = strdup(yytext); return SERVERTOKENS;}
fastcgi_pass	{yylval.str = strdup(yytext); return FASTCGIPASS;}
default_type	{yylval.str = strdup(yytext); return DEFAULTTYPE;}
server_name	{yylval.str = strdup(yytext); return SERVERNAME;}
ssl_dhparam	{yylval.str = strdup(yytext); return SSLDHPARAM;}
tcp_nopush	{yylval.str = strdup(yytext); return TCPNOPUSH;}
access_log	{yylval.str = strdup(yytext); return ACCESSLOG;}
log_format	{yylval.str = strdup(yytext); return LOGFORMAT;}
proxy_pass	{yylval.str = strdup(yytext); return PROXYPASS;}
error_page	{yylval.str = strdup(yytext); return ERRORPAGE;}
try_files	{yylval.str = strdup(yytext); return TRYFILES;}
error_log	{yylval.str = strdup(yytext); return ERRORLOG;}
autoindex	{return AUTOINDEX;}
reuseport	{yylval.str = strdup(yytext); return REUSEPORT;}
sendfile	{yylval.str = strdup(yytext); return SENDFILE;}
location	{yylval.str = strdup(yytext); return LOCATION;}
upstream	{yylval.str = strdup(yytext); return UPSTREAM;}
include		{yylval.str = strdup(yytext); return INCLUDE;}
expires		{yylval.str = strdup(yytext); return EXPIRES;}
rewrite		{yylval.str = strdup(yytext); return REWRITE;}
backup		{return BACKUP;}
events		{yylval.str = strdup(yytext); return EVENTS;}
server		{yylval.str = strdup(yytext); return SERVER;}
listen		{yylval.iValue = atoi(yytext); return LISTEN;}
return		{yylval.str = strdup(yytext); return RETURN;}
weight		{return WEIGHT;}
trace		{return TRACE;}
index		{yylval.str = strdup(yytext); return INDEX;}
http2:\/\/	{yylval.str = strdup(yytext); return HTTP2;}
http2		{yylval.str = strdup(yytext); return HTTP2L;}
https:\/\/	{yylval.str = strdup(yytext); return HTTPS;}
http:\/\/	{yylval.str = strdup(yytext); return HTTP1;}
http		{yylval.str = strdup(yytext); return HTTP;}
root		{yylval.str = strdup(yytext); return ROOT;}
main		{yylval.str = strdup(yytext); return MAIN;}
user		{yylval.str = strdup(yytext); return USER;}
pid			{yylval.str = strdup(yytext); return PID;}
ssl			{yylval.str = strdup(yytext); return SSL_;}
off			{yylval.str = strdup(yytext); return OFF;}
on			{yylval.str = strdup(yytext); return ON;}
;			{yylval.str = strdup(yytext); return EOL;}
[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+	{yylval.str = strdup(yytext); return IP;}
[0-9]+[kmgshd]	{yylval.str = strdup(yytext); return UNITS;}
:[0-9]+			{yylval.iValue = atoi(yytext+1); return PORT;}
[0-9]+			{yylval.iValue = atoi(yytext); return NUMBER;}
\*\.[A-Za-z0-9._-]+		{yylval.str = strdup(yytext); return PREFIXNAME;}
[A-Za-z0-9._-]+\.\*		{yylval.str = strdup(yytext); return SUFFIXNAME;}
\$[A-Za-z0-9._-]+\/*	{yylval.str = strdup(yytext); return VARIABLE;}
[A-Za-z0-9._-]+		{yylval.str = strdup(yytext); return NAME;}
[A-Za-z0-9._:-]+		{yylval.str = strdup(yytext); return COMPLEX;}
[A-Za-z0-9._/-]+	{yylval.str = strdup(yytext); return PATH;}
'[^';]*'		{yylval.str = strdup(yytext); return QUOTEDSTRING;}
(~|~\*|^~).*/\{	{yylval.str = strdup(yytext); return REGEXP;}
=			{return EQUAL_OPERATOR;}
\*			{return WILDCARD;}
#.*\n
[ \t\n]
.			{ return yytext[0]; }
%%
void yyerror( const char *s )
{
  fprintf( stderr ,"Line %d: %s\n", yylineno, s );
}

int yywrap(void) {
	return 1;
}
