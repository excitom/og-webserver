CFLAGS = -Wall -Wextra -lm
YFLAGS = -d --verbose
YACC = bison
LEX = flex

%.c: %.y

og_ws.tab.c: og_ws.y
	$(YACC) $(YFLAGS) og_ws.y

lex.yy.c:
	$(LEX) og_ws.l

LEXYACCSRC = og_ws.tab.c lex.yy.c

SRCS = og_ws.tab.c \
	ogws.c \
	serverState.c \
	getTimestamp.c \
	processInput.c \
	sendErrorResponse.c \
	handleGetVerb.c \
	handleProxyPass.c \
	handleFastCGIPass.c \
	handleTryFiles.c \
	getDocRoot.c \
	getUpstreamServer.c \
	parseMimeTypes.c \
	parseArgs.c \
	parseConfig.c \
	expandIncludeFiles.c \
	log.c \
	daemonize.c \
	showDirectoryListing.c \
	server.c \
	tlsServer.c \
	socket.c \
	lex.yy.c

OBJS = $(SRCS:.c=.o)
EXE = ogws

RELDIR = release
RELEXE = $(RELDIR)/$(EXE)
RELOBJS = $(addprefix $(RELDIR)/, $(OBJS))
RELCFLAGS = -O3 -DNDBG

DBGDIR = debug
DBGEXE = $(DBGDIR)/$(EXE)
DBGOBJS = $(addprefix $(DBGDIR)/, $(OBJS))
DBGCFLAGS = -O0 -DDBG -g
SSLFLAGS = -L/usr/local/lib -L /usr/local/lib64 -ldl -lpthread -lssl -lcrypto

debug: $(DBGEXE)

$(DBGEXE): $(DBGOBJS)
	cc $(CFLAGS) $(DBGCFLAGS) -o $(DBGEXE) $^ $(SSLFLAGS)

$(DBGDIR)/%.o: %.c
	cc -c $(CFLAGS) $(DBGCFLAGS) -o $@ $<

release: $(RELEXE) 

$(RELEXE): $(RELOBJS)
	cc $(CFLAGS) $(RELCFLAGS) -o $(RELEXE) $^ $(SSLFLAGS)

$(RELDIR)/%.o: %.c
	cc -c $(CFLAGS) $(RELCFLAGS) -o $@ $<

uninstall:
	rm -f /usr/local/bin/ogws
	rm -rf /etc/ogws
	rm -rf /var/www/ogws
	rm -rf /var/log/ogws
	userdel ogws

install:
	groupadd ogws
	useradd -d /var/www/ogws -c "OG Web Server" -M -g ogws -s /sbin/nologin ogws
	install -o ogws -g ogws -d /etc/ogws
	install -o ogws -g ogws -m 0644 ogws.conf.sample /etc/ogws/ogws.conf
	install -o ogws -g ogws -m 0644 mime.types /etc/ogws/mime.types
	touch /etc/ogws/ogws.pid
	chown ogws /etc/ogws/ogws.pid
	chgrp ogws /etc/ogws/ogws.pid
	chmod 0644 /etc/ogws/ogws.pid
	install -o ogws -g ogws -m 0755 -d /var/log/ogws
	install -o ogws -g ogws -m 0755 -d /var/www/ogws/html
	install -o ogws -g ogws -m 0644 -D html/* /var/www/ogws/html
	install -o ogws -g ogws -m 6755 release/ogws /usr/local/bin/ogws
	strip /usr/local/bin/ogws

debuginstall:
	install -o ec2-user -g ec2-user -d /etc/ogws
	install -o ec2-user -g ec2-user -m 0644 ogws.conf.debug /etc/ogws/ogws.conf
	install -o ec2-user -g ec2-user -m 0644 mime.types /etc/ogws/mime.types
	touch /etc/ogws/ogws.pid
	chown ec2-user /etc/ogws/ogws.pid
	chgrp ec2-user /etc/ogws/ogws.pid
	chmod 0644 /etc/ogws/ogws.pid
	install -o ec2-user -g ec2-user -d /var/log/ogws
	install -o ec2-user -g ec2-user -d /var/www/ogws/html
	install -o ec2-user -g ec2-user -D html/* /var/www/ogws/html
	install -o ec2-user -g ec2-user debug/ogws /usr/local/bin/ogws

clean:
	rm -f $(RELEXE) $(RELOBJS) $(DBGEXE) $(DBGOBJS)
	rm -f lex.yy.*
	rm -f og_ws.tab.*
	rm -f og_ws.output $(LEXYACCSRC)
