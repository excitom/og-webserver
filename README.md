# og-webserver
This is an event driven web server implementation using a single process and `epoll` to asynchronously multiplex socket connections.

This is a personal interest project that I am incrementally improving.

The *og* could be "old guy" since I'm an old guy, or "original gangster"
since I wrote a lot of operating system and network level code back around 
the turn of the century, but my career evolved into web development (PHP,
javascript, CSS) and subsequently engineering management. I'm having fun with
this project, revisiting old skills.

Note: While implementing SSL I ran into trouble getting `SSL_accept`
to work with `epoll` and asynchronous I/O, so I switched to a
multi-threaded server implementation with synchronous I/O when 
serving SSL. This is a work in progress.
