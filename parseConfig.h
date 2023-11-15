// config parser functions
// which correspond to yacc parser actions.
void f_pid(char *);
void f_include(char *);
void f_trace(int);
void f_autoindex(int);
void f_sendfile(int);
void f_tcpnopush(int);
void f_user(char *, char *);
void f_server();
void f_http();
void f_root(char *);
void f_expires(char *);
void f_server_name(char *, int);
void f_indexFile(char *);
void f_error_log(char *);
void f_access_log(char *, int);
void f_ssl_certificate(char *);
void f_ssl_certificate_key(char *);
void f_listen(char *, int);
void f_tls();
void f_location(int, char *);
void f_upstreams(char *);
void f_upstream(char *, int, int);
void f_default_type(char *);
void f_try_files();
void f_try_target(char *);
void f_proxy_pass(char *, int);
void f_protocol(char *);
void f_fastcgi_pass(char *, int);
void f_fastcgi_index(char *);
void f_fastcgi_param(char *, char *);
void f_fastcgi_split_path_info(char *);
void f_keepalive_timeout(int);
void f_workerProcesses(int);
void f_workerConnections(int);
void f_events();
void f_config_complete();
// utility functions for config file parsing
void defaultAccessLog();
void defaultErrorLog();
void defaultPort();
void defaultServerName();
void defaultLocation();
void defaultIndexFile();
void defaultType();
void defaultUpstreams();
void checkConfig();
void checkServers();
void checkDocRoots(_server *);
void checkServerNames(_server *);
void checkIndexFiles(_server *);
void checkAccessLogs(_server *);
void checkErrorLogs(_server *);
void checkCertFile(_server *);
void checkKeyFile(_server *);
int checkPorts(int, int);
int portOk(_server *);
int pathAlreadyOpened(const char *, _log_file *);
void openLogFiles();
_upstreams *isUpstreamGroup(char *);
void proxyPassToUpstgreamGroup(int, char *, _upstreams *);
void proxyPassToHost(int, char *, int);
