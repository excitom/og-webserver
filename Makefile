CFLAGS = 

SRCS = server.c \
	  getTimestamp.c \
	  processInput.c \
	  sendErrorResponse.c \
	  handleGetVerb.c \
	  parseMimeTypes.c \
	  parseArgs.c \
	  log.c \
	  daemonize.c

OBJS = $(SRCS:.c=.o)
EXE = server

RELDIR = release
RELEXE = $(RELDIR)/$(EXE)
RELOBJS = $(addprefix $(RELDIR)/, $(OBJS))
RELCFLAGS = -O3 -DNDBG

DBGDIR = debug
DBGEXE = $(DBGDIR)/$(EXE)
DBGOBJS = $(addprefix $(DBGDIR)/, $(OBJS))
DBGCFLAGS = -O0 -DDBG -g

debug: $(DBGEXE)

$(DBGEXE): $(DBGOBJS)
	cc $(CFLAGS) $(DBGCFLAGS) -o $(DBGEXE) $^

$(DBGDIR)/%.o: %.c
	cc -c $(CFLAGS) $(DBGCFLAGS) -o $@ $<

release: $(RELEXE)

$(RELEXE): $(RELOBJS)
	cc $(CFLAGS) $(RELCFLAGS) -o $(RELEXE) $^

$(RELDIR)/%.o: %.c
	cc -c $(CFLAGS) $(RELCFLAGS) -o $@ $<

clean:
	rm -f $(RELEXE) $(RELOBJS) $(DBGEXE) $(DBGOBJS)
