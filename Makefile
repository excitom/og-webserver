objects = server.o getTimestamp.o
all: $(objects)

$(objects): %.o: %.c
	cc -c $< -o $@

server: $(objects)
	cc -o server $(objects)

debug:
	cc -g -o server server.c

clean:
	rm -f *.o
	rm -f server
