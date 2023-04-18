CFLAGS = -Wall -Wextra

SRCS = ogws.c \
	  getTimestamp.c \
	  processInput.c \
	  sendErrorResponse.c \
	  handleGetVerb.c \
	  handleProxyPass.c \
	  getDocRoot.c \
	  parseMimeTypes.c \
	  parseArgs.c \
	  parseConfig.c \
	  log.c \
	  daemonize.c \
	  showDirectoryListing.c \
	  server.c \
	  tlsServer.c \
	  socket.c

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

clean:
	rm -f $(RELEXE) $(RELOBJS) $(DBGEXE) $(DBGOBJS)
