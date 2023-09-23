<img src="html/ogws-logo.png" width="150"/>

# og-webserver

This is an event driven web server implementation.

This is a personal interest project that I am incrementally improving.

The *og* could be "old guy" since I'm an old guy, or "original gangster"
since I wrote a lot of operating system and network level code back around 
the turn of the century, but my career evolved into web development (PHP,
javascript, CSS) and subsequently engineering management. I'm having fun with
this project, revisiting old skills.

## Implementation Language

In the spirit of "old school" I went with C as the implementation language. 

## Event Driven Server

The main loop in the web server uses `epoll` to monitor for incoming web requests. 

Note: While implementing SSL I ran into trouble getting `SSL_accept`
to work with `epoll` and asynchronous I/O, so I switched to a
multi-threaded server implementation with synchronous I/O when 
serving SSL. This is a work in progress. However it makes the code interesting as it
demonstrates two completely different approaches to handle requests.

The server can be configured to use multiple processes, to improve scalability.

If you configure both an HTTP and HTTPS virtual host, they will happily coexist using both `epoll` and threads.

## NGINX-compatible Config File

While designing the way to configure the server I decided to use same directives and format
as NGINX rather than re-invent the wheel. In fact, I use sample config files from the NGINX 
documentation as test cases for my config parsing.

To do the work of parsing I use the `lex` and `yacc` tools.

## Multi-process and Multi-thread Operations

If you configure multiple worker processes then each is a full clone of the 
complete configuration, running independantly and in parallel. There isn't 
any shared memory or other context. Some day if I implement cacheing I will
use something like `memcached` for the shared memory.

Similarly, each process has a complete, independant copy of the thread 
environment, when supporting a TLS/SSL server.

## Web Server, API Gateway, and Load Balancer Features

The server can handle `GET` requests for local files. `GET`, `POST`, `PUT`, and `DELETE` can be handled by proxy-pass to a single server or an upstream server group.

The implementation of upstream group allows for load balancer function.
The servers in an upstream group can have a weight applied to send more traffic, otherwise the traffic is distributed equal round-robin to the upstream servers.

## Local File Serving

The `try files` feature from NGINX is implmented for flexible local file
handling. For example:

`
  location / {
    try_files $uri $uri/ /index.php;
  }`

This directive first checks if the URI corresponds to a path in the local
filesystem. If so, that file is served. If not, the server next checks
if the URI corresponds to a directory name. If so, the index file for that
directory is served. If neither condition holds, then serve the index file
from the `docroot`. The URI is passed to the index file as a parameter
so that business logic in the file can decide what content to serve.

## Directory Indexing

Typically, a `404` HTTP return code is served if a client requests a 
directory name that does not contain an index file. However, the config
file can override this behavior and the server will return an HTML page
containing a listing of the files in the directory.

## Regular Expression Matching

Some of the config file directives utilize regular expression matching.
These are implemented using the `regex` C library.

