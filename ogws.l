%{
#include <stdio.h>
#include "ogws.tab.h"
extern int main(int, char **);
extern YYSTYPE yylval;
%}
%option yylineno
%%
server_names_hash_bucket_size {yylval.iValue = atoi(yytext); return HASHBUCKET;}
worker_rlimit_nofile {yylval.iValue = atoi(yytext); return WORKERRLIMIT;}
ssl_certificate_key	{yylval.str = strdup(yytext); return SSLCERTIFICATEKEY;}
worker_connections	{yylval.iValue = atoi(yytext); return WORKERCONNECTIONS;}
keepalive_timeout	{yylval.iValue = atoi(yytext); return KEEPALIVETIMEOUT;}
worker_processes	{yylval.iValue = atoi(yytext); return WORKERPROCESSES;}
ssl_certificate	{yylval.str = strdup(yytext); return SSLCERTIFICATE;}
default_server	{yylval.str = strdup(yytext); return DEFAULTSERVER;}
log_not_found	{yylval.str = strdup(yytext); return LOGNOTFOUND;}
fastcgi_pass	{yylval.str = strdup(yytext); return FASTCGIPASS;}
default_type	{yylval.str = strdup(yytext); return DEFAULTTYPE;}
server_name	{yylval.str = strdup(yytext); return SERVERNAME;}
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
events		{yylval.str = strdup(yytext); return EVENTS;}
server		{yylval.str = strdup(yytext); return SERVER;}
listen		{yylval.iValue = atoi(yytext); return LISTEN;}
return		{yylval.str = strdup(yytext); return RETURN;}
weight		{return WEIGHT;}
user		{yylval.str = strdup(yytext); return USER;}
pid			{yylval.str = strdup(yytext); return PID;}
http2:\/\/	{yylval.str = strdup(yytext); return HTTP2;}
http2		{yylval.str = strdup(yytext); return HTTP2L;}
https:\/\/	{yylval.str = strdup(yytext); return HTTPS;}
http:\/\/	{yylval.str = strdup(yytext); return HTTP1;}
http		{yylval.str = strdup(yytext); return HTTP;}
root		{yylval.str = strdup(yytext); return ROOT;}
trace		{return TRACE;}
index		{yylval.str = strdup(yytext); return INDEX;}
main		{yylval.str = strdup(yytext); return MAIN;}
ssl			{yylval.str = strdup(yytext); return SSL;}
on			{yylval.str = strdup(yytext); return ON;}
off			{yylval.str = strdup(yytext); return OFF;}
;			{yylval.str = strdup(yytext); return EOL;}
(=|~|~\*|^~)			{yylval.str = strdup(yytext); return LOC_OPERATOR;}
[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+	{yylval.str = strdup(yytext); return IP;}
[0-9]+[kmgsd]		{yylval.str = strdup(yytext); return UNITS;}
:[0-9]+			{yylval.iValue = atoi(yytext+1); return PORT;}
[0-9]+			{yylval.iValue = atoi(yytext); return NUMBER;}
\*\.[A-Za-z0-9._-]+		{yylval.str = strdup(yytext); return PREFIXNAME;}
[A-Za-z0-9._-]+\.\*		{yylval.str = strdup(yytext); return SUFFIXNAME;}
[A-Za-z0-9._-]+		{yylval.str = strdup(yytext); return NAME;}
[A-Za-z0-9._/-]+	{yylval.str = strdup(yytext); return PATH;}
~.[^{]+		{yylval.str = strdup(yytext); return REGEXP;}
'[^';]*'		{yylval.str = strdup(yytext); return QUOTEDSTRING;}
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
