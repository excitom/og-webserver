%{
#include <stdio.h>
#include "ogws.tab.h"
extern int main(int, char **);
extern YYSTYPE yylval;
%}
%option yylineno
%%
user			{yylval.str = strdup(yytext); return USER;}
error_log		{yylval.str = strdup(yytext); return ERRORLOG;}
access_log		{yylval.str = strdup(yytext); return ACCESSLOG;}
worker_processes	{yylval.iValue = atoi(yytext); return WORKERPROCESSES;}
include			{yylval.str = strdup(yytext); return INCLUDE;}
pid			{yylval.str = strdup(yytext); return PID;}
events			{yylval.str = strdup(yytext); return EVENTS;}
worker_connections	{yylval.iValue = atoi(yytext); return WORKERCONNECTIONS;}
http2:			{yylval.str = strdup(yytext); return HTTP2;}
https:			{yylval.str = strdup(yytext); return HTTPS;}
http:			{yylval.str = strdup(yytext); return HTTP1;}
http			{yylval.str = strdup(yytext); return HTTP;}
keepalive_timeout	{yylval.iValue = atoi(yytext); return KEEPALIVETIMEOUT;}
default_type		{yylval.str = strdup(yytext); return DEFAULTTYPE;}
server			{yylval.str = strdup(yytext); return SERVER;}
sendfile		{yylval.str = strdup(yytext); return SENDFILE;}
tcp_nopush		{yylval.str = strdup(yytext); return TCPNOPUSH;}
listen			{yylval.iValue = atoi(yytext); return LISTEN;}
server_name		{yylval.str = strdup(yytext); return SERVERNAME;}
default_server		{yylval.str = strdup(yytext); return DEFAULTSERVER;}
location		{yylval.str = strdup(yytext); return LOCATION;}
root			{yylval.str = strdup(yytext); return ROOT;}
proxy_pass		{yylval.str = strdup(yytext); return PROXYPASS;}
return			{yylval.str = strdup(yytext); return RETURN;}
expires			{yylval.str = strdup(yytext); return EXPIRES;}
rewrite			{yylval.str = strdup(yytext); return REWRITE;}
error_page		{yylval.str = strdup(yytext); return ERRORPAGE;}
log_not_found		{yylval.str = strdup(yytext); return LOGNOTFOUND;}
try_files		{yylval.str = strdup(yytext); return TRYFILES;}
ssl_certificate_key	{yylval.str = strdup(yytext); return SSLCERTIFICATEKEY;}
ssl_certificate		{yylval.str = strdup(yytext); return SSLCERTIFICATE;}
ssl			{yylval.str = strdup(yytext); return SSL;}
trace			{return TRACE;}
autoindex		{return AUTOINDEX;}
reuseport		{yylval.str = strdup(yytext); return REUSEPORT;}
index			{yylval.str = strdup(yytext); return INDEX;}
on			{yylval.str = strdup(yytext); return ON;}
off			{yylval.str = strdup(yytext); return OFF;}
main			{yylval.str = strdup(yytext); return MAIN;}
~			{yylval.str = strdup(yytext); return TILDE;}
;			{yylval.str = strdup(yytext); return EOL;}
:			{yylval.str = strdup(yytext); return COLON;}
=			{yylval.str = strdup(yytext); return EQUAL;}
\/			{yylval.str = strdup(yytext); return SLASH;}
\*			{yylval.str = strdup(yytext); return STAR;}
[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+	{yylval.str = strdup(yytext); return IP;}
[0-9]+[kmgs]		{yylval.str = strdup(yytext); return UNITS;}
[0-9]+			{yylval.iValue = atoi(yytext); return NUMBER;}
\*\.[A-Za-z0-9._-]+		{yylval.str = strdup(yytext); return PREFIXNAME;}
[A-Za-z0-9._-]+\.\*		{yylval.str = strdup(yytext); return SUFFIXNAME;}
[A-Za-z0-9._-]+		{yylval.str = strdup(yytext); return NAME;}
[A-Za-z0-9._/-]+	{yylval.str = strdup(yytext); return PATH;}
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
