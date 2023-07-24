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

## NGINX-compatible Config File

While designing the way to configure the server I decided to use same directives and format
as NGINX rather than re-invent the wheel. In fact, I use sample config files from the NGINX 
documentation as test cases for my config parsing.

To do the work of parsing I use the `lex` and `yacc` tools.
